#include "app_gen_mesh.h"
#include <stb_image.h>
#include <limbrary/model_view/code_mesh.h>
#include <limbrary/model_view/model_loader.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>


namespace lim
{
	AppGenMesh::AppGenMesh() : AppBase(1200, 780, APP_NAME)
	{
		glEnable(GL_CULL_FACE);
		// glCullFace(GL_FRONT);
		// glPolygonMode(GL_FRONT, GL_LINE);
		stbi_set_flip_vertically_on_load(true);

		program = new Program("debugging", APP_DIR);
		program->attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();

		viewport = new ViewportWithCamera(new MsFramebuffer());

		/* gen models */
		models.push_back(new Model(code_mesh::genSphere(50, 25), "sphere"));

		models.push_back(new Model(code_mesh::genDonut(50, 25), "donut"));
		models.push_back(new Model(code_mesh::genCapsule(50, 25), "capsule"));

		models.push_back(new Model(code_mesh::genIcoSphere(0), "ico sphere"));
		models.push_back(new Model(code_mesh::genIcoSphere(3), "ico sphere2"));
		models.push_back(new Model(code_mesh::genCubeSphere(2), "cube sphere2"));
		models.push_back(new Model(code_mesh::genCubeSphere(5), "cube sphere2"));
		models.push_back(new Model(code_mesh::genCubeSphere2(5), "cube sphere2"));
		// models.back()->meshes.back()->drawMode = GL_LINE_LOOP;

		models.push_back(new Model(code_mesh::genQuad(), "quad"));

		models.push_back(new Model(code_mesh::genCube(), "cube"));

		models.push_back(new Model(code_mesh::genCylinder(20), "cylinder"));

		models.push_back(loadModelFromFile("assets/models/objs/spot.obj", true));

		models.push_back(loadModelFromFile("assets/models/objs/Wooden Crate.obj", true));

		const float interModels = 3.5f;
		const float biasModels = -interModels * models.size() / 2.f;

		models[0]->position = {biasModels - interModels, 0, 0};
		models[0]->updateModelMat();

		for (int i = 1; i < models.size(); i++)
		{
			models[i]->position = {biasModels + interModels * i, 0, 0};
			models[i]->updateModelMat();
		}

		models.push_back(new Model(code_mesh::genPlane(), "plane"));
		models.back()->position = glm::vec3(0, -3.5, 0);
		models.back()->scale = glm::vec3(50.f);
		models.back()->updateModelMat();

		light = new Light();
		light->distance = 10.f;
		light_model = new Model(code_mesh::genSphere(8, 4), "sphere");
		light_model->position = light->position;
		light_model->scale = glm::vec3(0.3f);
		light_model->updateModelMat();

		debugging_tex = new TexBase(GL_SRGB8);
		debugging_tex->wrap_param = GL_REPEAT;
		loadImageToTex("assets/images/uv_grid.jpg", *debugging_tex);
	}
	AppGenMesh::~AppGenMesh()
	{
		delete program;
		delete viewport;
		for (Model *m : models)
		{
			delete m;
		}
		delete light;
		delete light_model;
	}

	void AppGenMesh::update()
	{
		/* render to fbo in viewport */
		viewport->framebuffer->bind();

		Program &prog = *program;
		prog.use();

		// camera->printCameraState();
		const Camera& camera = viewport->camera;
		prog.setUniform("viewMat", camera.view_mat);
		prog.setUniform("projMat", camera.proj_mat);
		prog.setUniform("cameraPos", camera.position);

		prog.setUniform("lightDir", light->direction);
		prog.setUniform("lightColor", light->color);
		prog.setUniform("lightInt", light->intensity);
		prog.setUniform("lightPos", light->position);

		prog.setUniform("time", vs_t);

		glActiveTexture(GL_TEXTURE31);
		glBindTexture(GL_TEXTURE_2D, debugging_tex->tex_id);
		prog.setUniform("uvgridTex", 31);

		prog.setUniform("modelMat", light_model->model_mat);
		Mesh* mesh = light_model->meshes.back();
		glBindVertexArray(mesh->VAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
		glDrawElements(mesh->draw_mode, static_cast<GLuint>(mesh->indices.size()), GL_UNSIGNED_INT, 0);

		for (Model *m : models)
		{
			Mesh* mesh = m->meshes.back();
			prog.setUniform("modelMat", m->model_mat);
			glBindVertexArray(mesh->VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->EBO);
			glDrawElements(mesh->draw_mode, static_cast<GLuint>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
		}

		viewport->framebuffer->unbind();

		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		processInput(window);
	}
	void AppGenMesh::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		viewport->drawImGui();

		/* controller */
		if (false && ImGui::Begin("controller##genMesh"))
		{

			ImGui::SliderFloat("vs_t", &vs_t, 0.f, 1.f);

			ImGui::Text("<light>");
			const float yawSpd = 360 * 0.001;
			const float pitchSpd = 80 * 0.001;
			const float distSpd = 94 * 0.001;
			bool isDraging = ImGui::SliderFloat("yaw", &light->yaw, 0, 360, "%.3f");
			isDraging |= ImGui::SliderFloat("pitch", &light->pitch, 10, 90, "%.3f");
			isDraging |= ImGui::SliderFloat("distance", &light->distance, 6, 100, "%.3f");
			if (isDraging)
			{
				light->updateMembers();
				light_model->position = light->position;
				light_model->updateModelMat();
			}
			ImGui::Text("pos %f %f %f", light->position.x, light->position.y, light->position.z);

			ImGui::End();
		}

		/* state view */
		if (ImGui::Begin("camera##genMesh"))
		{
			static float rad = F_PI;
			ImGui::SliderAngle("rad", &rad);
			ImGui::Text("%d", viewport->camera.viewing_mode);
			ImGui::End();
		}
	}
	void AppGenMesh::processInput(GLFWwindow *window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
}