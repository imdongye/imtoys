#include "app_gen_mesh.h"
#include <stb_image.h>
#include <limbrary/model_view/code_mesh.h>
#include <limbrary/model_view/renderer.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <limbrary/asset_lib.h>

namespace
{
	void align(std::vector<float> v)
	{

	}
}

namespace lim
{
	AppGenMesh::AppGenMesh() : AppBase(1200, 780, APP_NAME)
	{
		glEnable(GL_CULL_FACE);
		// glCullFace(GL_FRONT);
		// glPolygonMode(GL_FRONT, GL_LINE);
		stbi_set_flip_vertically_on_load(true);

		viewport = new ViewportWithCamera("viewport##gen_mesh", new MsFramebuffer());
		viewport->camera.move_free_spd = 4.f;
		/* gen models */
		models.push_back(new Model("sphere"));
		models.back()->meshes.push_back(code_mesh::genSphere(50, 25));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("donut"));
		models.back()->meshes.push_back(code_mesh::genDonut(50, 25));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("capsule"));
		models.back()->meshes.push_back(code_mesh::genCapsule(50, 25));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("ico sphere"));
		models.back()->meshes.push_back(code_mesh::genIcoSphere(0));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("ico sphere2"));
		models.back()->meshes.push_back(code_mesh::genIcoSphere(3));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("cube sphere"));
		models.back()->meshes.push_back(code_mesh::genCubeSphere(2));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("cube sphere2"));
		models.back()->meshes.push_back(code_mesh::genCubeSphere2(5));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("quad"));
		models.back()->meshes.push_back(code_mesh::genQuad());
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("cube"));
		models.back()->meshes.push_back(code_mesh::genCube());
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(new Model("cylinder"));
		models.back()->meshes.push_back(code_mesh::genCylinder(20));
		models.back()->root.meshes.push_back(models.back()->meshes.back());

		models.push_back(importModelFromFile("assets/models/objs/spot.obj", true, false));

		models.push_back(importModelFromFile("assets/models/objs/Wooden Crate.obj", true, false));

		const float interModels = 3.5f;
		const float biasModels = -interModels * models.size() / 2.f;

		models[0]->position = {biasModels - interModels, 0, 0};
		models[0]->updateModelMat();

		for( int i = 1; i < models.size(); i++ )
		{
			models[i]->position = {biasModels + interModels * i, 0, 0};
			models[i]->updateModelMat();
		}

		models.push_back(new Model("plane"));
		models.back()->meshes.push_back(code_mesh::genPlane());
		models.back()->root.meshes.push_back(models.back()->meshes.back());
		models.back()->position = glm::vec3(0, -3.5, 0);
		models.back()->scale = glm::vec3(50.f);
		models.back()->updateModelMat();

		light = new Light();
		light->distance = 10.f;
		light_model = new Model("light model");
		light_model->meshes.push_back(code_mesh::genSphere(8, 4));
		light_model->root.meshes.push_back(models.back()->meshes.back());
		light_model->position = light->position;
		light_model->scale = glm::vec3(0.3f);
		light_model->updateModelMat();

		scene.lights.push_back(light);
		scene.models = models;

		debugging_tex = new Texture();
		debugging_tex->initFromImage("assets/images/uv_grid.jpg", GL_SRGB8);
		debugging_tex->wrap_param = GL_REPEAT;

		program = new Program("debugging", APP_DIR);
		program->attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();
		program->use_hook = [this](const Program& prog) {
			prog.setUniform("time", vs_t);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, debugging_tex->tex_id);
			prog.setUniform("uvgridTex", 0);
		};
		AssetLib::get().default_mat.prog = program;
	}
	AppGenMesh::~AppGenMesh()
	{
		delete program;
		delete viewport;
		for(Model* md : models) 
			delete md;
		delete light;
		delete light_model;
		delete debugging_tex;
	}

	void AppGenMesh::update()
	{
		/* render to fbo in viewport */
		render(*viewport->framebuffer, viewport->camera, scene);

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