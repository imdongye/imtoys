//
//  framework template
//	2023-02-08 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_MESH_H
#define APP_MESH_H

#include "../limbrary/limclude.h"

namespace lim
{
	class AppGenMesh: public AppBase
	{
	public:
		inline static constexpr const char *APP_DIR = "imtests/";
		inline static constexpr const char *APP_NAME = "mesh gen tester";
		inline static constexpr const char *APP_DISC = "hello, world";
	private:
		// ui vars
		glm::vec2 uv_scale ={1.f, 1.f};
		float vs_t = 0.f;

		bool start_dragging = false;
		Camera *camera;
		Program *program;
		Viewport *viewport;
		std::vector<Model*> models;
		Light *light;
		Model *light_model;
		TexBase *debugging_tex;

	public:
		AppGenMesh(): AppBase(1480, 720, APP_NAME)
		{
			glEnable(GL_CULL_FACE);
			//glCullFace(GL_FRONT);
			//glPolygonMode(GL_FRONT, GL_LINE);
			stbi_set_flip_vertically_on_load(true);

			camera = new Camera(glm::vec3(0, 0, 8), win_width/(float)win_height);
			camera->fovy = 45.f;
			camera->updateProjMat();

			program = new Program("debugging", APP_DIR);
			program->attatch("debug.vs").attatch("debug.fs").link();

			viewport = new Viewport(new MsFramebuffer());
			viewport->framebuffer->clear_color ={0.1f, 0.1f, 0.1f, 1.0f};

			/* gen models */
			models.push_back(new Model(MeshGenerator::genSphere(50, 25), "sphere"));

			
			models.push_back(new Model(MeshGenerator::genDonut(50, 25), "donut"));
			models.push_back(new Model(MeshGenerator::genCapsule(50, 25), "capsule"));

			models.push_back(new Model(MeshGenerator::genIcoSphere(0), "ico sphere"));
			models.push_back(new Model(MeshGenerator::genIcoSphere(3), "ico sphere2"));
			models.push_back(new Model(MeshGenerator::genCubeSphere(2), "cube sphere2"));
			models.push_back(new Model(MeshGenerator::genCubeSphere(5), "cube sphere2"));
			models.push_back(new Model(MeshGenerator::genCubeSphere2(5), "cube sphere2"));
			//models.back()->meshes.back()->drawMode = GL_LINE_LOOP;

			models.push_back(new Model(MeshGenerator::genQuad(), "quad"));

			models.push_back(new Model(MeshGenerator::genCube(), "cube"));

			models.push_back(new Model(MeshGenerator::genCylinder(20), "cylinder"));

			models.push_back(ModelLoader::loadFile("common/archive/dwarf/Dwarf_2_Low.obj", true));

			models.push_back(ModelLoader::loadFile("common/archive/meshes/stanford-bunny.obj", true));
			

			const float interModels = 3.5f;
			const float biasModels = -interModels*models.size()/2.f;

			models[0]->position ={biasModels-interModels, 0, 0};
			models[0]->updateModelMat();

			for( int i = 1; i<models.size(); i++ ) {
				models[i]->position ={biasModels+interModels*i, 0, 0};
				models[i]->updateModelMat();
			}

			light = new Light();
			light->distance = 10.f;
			light_model = new Model(MeshGenerator::genSphere(8, 4), "sphere");
			light_model->position = light->position;
			light_model->scale = glm::vec3(0.3f);
			light_model->updateModelMat();

			debugging_tex = new TexBase(GL_SRGB8);
			debugging_tex->wrap_param = GL_REPEAT;
			loadImageToTex("common/images/uv_grid.jpg", *debugging_tex);
		}
		~AppGenMesh()
		{
			delete camera;
			delete program;
			delete viewport;
			for( Model* m : models ) {
				delete m;
			}
			delete light;
			delete light_model;
		}
	private:
		virtual void update() final
		{
			processInput(window);

			/* render to fbo in viewport */
			viewport->framebuffer->bind();

			Program& prog = *program;
			prog.use();

			camera->aspect = viewport->framebuffer->aspect;
			camera->updateProjMat();
			prog.setUniform("viewMat", camera->view_mat);
			prog.setUniform("projMat", camera->proj_mat);
			prog.setUniform("cameraPos", camera->position);

			light->setUniforms(prog);

			prog.setUniform("time", vs_t);

			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, debugging_tex->tex_id);
			prog.setUniform("uvgridTex", 31);

			prog.setUniform("modelMat", light_model->model_mat);
			light_model->meshes.back()->draw();

			for( Model* m : models ) {
				prog.setUniform("modelMat", m->model_mat);
				m->meshes.back()->draw();
			}

			viewport->framebuffer->unbind();

			// clear backbuffer
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, fb_width, fb_height);
			glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		virtual void renderImGui() final
		{
			ImGui::DockSpaceOverViewport();


			viewport->drawImGui();

			/* controller */
			ImGui::Begin("controller##genMesh");

			ImGui::SliderFloat("vs_t", &vs_t, 0.f, 1.f);

			ImGui::Text("<light>");
			const float yawSpd = 360 * 0.001;
			const float pitchSpd = 80 * 0.001;
			const float distSpd = 94 * 0.001;
			bool isDraging = ImGui::SliderFloat("yaw", &light->yaw, 0, 360, "%.3f");
			isDraging |= ImGui::SliderFloat("pitch", &light->pitch, 10, 90, "%.3f");
			isDraging |= ImGui::SliderFloat("distance", &light->distance, 6, 100, "%.3f");
			if( isDraging ) {
				light->updateMembers();
				light_model->position = light->position;
				light_model->updateModelMat();
			}
			ImGui::Text("pos %f %f %f", light->position.x, light->position.y, light->position.z);

			ImGui::End();

			/* state view */
			ImGui::Begin("state##genMesh");
			ImGui::Text("fovy: %f", camera->fovy);
			ImGui::End();
		}

	private:
		void processInput(GLFWwindow *window)
		{
			static const double cameraMoveSpeed = 4.2;
			if( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS )
				glfwSetWindowShouldClose(window, true);

			if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::FORWARD, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::BACKWARD, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::LEFT, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::RIGHT, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::UP, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::DOWN, delta_time, cameraMoveSpeed);
			camera->updateFreeViewMat();
		}
		/* glfw callback */
		virtual void framebufferSizeCallback(double width, double height) final
		{
			glViewport(0, 0, width, height);
		}
		virtual void scrollCallback(double xoff, double yoff) final
		{
			if( !viewport->focused ) return;
			camera->shiftZoom(yoff*5.f);
			camera->updateProjMat();
		}
		virtual void mouseBtnCallback(int button, int action, int mods) final
		{
			start_dragging = action==GLFW_PRESS;
		}
		virtual void cursorPosCallback(double xpos, double ypos) final
		{
			static const float rotateSpeed = 0.08f;
			static float xoff, yoff;
			static double xold, yold;

			if( !viewport->dragging )return;

			if( start_dragging ) {
				xold = xpos;
				yold = ypos;
				start_dragging = false;
			}

			xoff = xpos - xold;
			yoff = yold - ypos;

			xold = xpos;
			yold = ypos;

			camera->rotateCamera(xoff * rotateSpeed, yoff * rotateSpeed);
			camera->updateFreeViewMat();
		}
	};
}

#endif
