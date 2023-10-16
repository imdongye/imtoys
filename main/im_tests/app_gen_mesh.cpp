#include "app_gen_mesh.h"
#include <stb_image.h>
#include <limbrary/model_view/mesh_maked.h>
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
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		// glPolygonMode(GL_FRONT, GL_LINE);
		viewport = new ViewportWithCamera("viewport##gen_mesh", new FramebufferMs());
		viewport->camera.spd_free_move = 4.f;
		/* gen models */
		models.push_back(new Model("sphere"));
		models.back()->my_meshes.push_back(new MeshSphere(50, 25));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("donut"));
		models.back()->my_meshes.push_back(new MeshDonut(50, 25));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("capsule"));
		models.back()->my_meshes.push_back(new MeshCapsule(50, 25));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("ico sphere"));
		models.back()->my_meshes.push_back(new MeshIcoSphere(0));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("ico sphere2"));
		models.back()->my_meshes.push_back(new MeshIcoSphere(1)); // 구멍뭐지
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("ico sphere2"));
		models.back()->my_meshes.push_back(new MeshIcoSphere(2));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("ico sphere2"));
		models.back()->my_meshes.push_back(new MeshIcoSphere(3));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("cube sphere"));
		models.back()->my_meshes.push_back(new MeshCubeSphere(2));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("cube sphere2"));
		models.back()->my_meshes.push_back(new MeshCubeSphere2(5));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("quad"));
		models.back()->my_meshes.push_back(new MeshQuad());
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("cube"));
		models.back()->my_meshes.push_back(new MeshCube());
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model("cylinder"));
		models.back()->my_meshes.push_back(new MeshCylinder(20));
		models.back()->root.addMesh(models.back()->my_meshes.back());

		models.push_back(new Model());
		models.back()->importFromFile("assets/models/objs/spot.obj", true, false);

		models.push_back(new Model());
		models.back()->importFromFile("assets/models/objs/Wooden Crate.obj", true, false);

		const float interModels = 3.5f;
		const float biasModels = -interModels * models.size() / 2.f;

		for( int i = 0; i < models.size(); i++ )
		{
			models[i]->position = {biasModels + interModels * i, 0, 0};
			models[i]->updateModelMat();
		}

		models.push_back(new Model("plane"));
		models.back()->my_meshes.push_back(new MeshPlane());
		models.back()->root.addMesh(models.back()->my_meshes.back());
		models.back()->position = glm::vec3(0, -3.5, 0);
		models.back()->scale = glm::vec3(50.f);
		models.back()->updateModelMat();

		light = new Light();
		light_model = new Model("light model");
		light_model->my_meshes.push_back(new MeshSphere(8, 4));
		light_model->root.addMesh(models.back()->my_meshes.back());
		light_model->position = light->position;
		light_model->scale = glm::vec3(0.3f);
		light_model->updateModelMat();

		scene.lights.push_back(light);
		for(const Model* md : models) {
			scene.models.push_back(md);
		}

		debugging_tex = new Texture();
		debugging_tex->initFromImage("assets/images/uv_grid.jpg", GL_SRGB8);
		debugging_tex->wrap_param = GL_REPEAT;

		program = new Program();
		program->name = "debugging";
		program->home_dir = APP_DIR;
		program->attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();
		AssetLib::get().default_material.prog = program;
		AssetLib::get().default_material.set_program = [this](const Program& prog) {
			prog.setUniform("time", vs_t);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, debugging_tex->tex_id);
			prog.setUniform("uvgridTex", 0);
		};
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
		render(viewport->getFb(), viewport->camera, scene);

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
			const static float litThetaSpd = 70 * 0.001;
			const static float litPiSpd = 360 * 0.001;
			static float litTheta = 30.f;
			static float litPi = 30.f;
			if( ImGui::DragFloat("light yaw", &litPi, litPiSpd, 0, 360, "%.3f") ||
				ImGui::DragFloat("light pitch", &litTheta, litThetaSpd, 0, 180, "%.3f") ) {

				light->setRotate(litTheta, litPi);
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
			ImGui::End();
		}
	}
	void AppGenMesh::processInput(GLFWwindow *window)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
	}
}