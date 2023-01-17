//
//  for aplicate real-time hatching paper
//	2023-1-17 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_HACHING_H
#define APP_HACHING_H

namespace lim
{
	class AppHatching: public AppBase
	{
	public:
		inline static constexpr const char *APP_DIR = "imnpr/";
		inline static constexpr const char *APP_NAME = "real-time hatching";
		inline static constexpr const char *APP_DISC = "aplicate real-time hatching paper";
	private:
		bool start_dragging = false;
		Camera *camera;
		Program *program;
		Viewport *viewport;
		std::vector<Model*> models;
		Mesh *sphere;
		Light *light;

	public:
		AppHatching(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			camera = new Camera(glm::vec3(0, 0, 12), scr_width/(float)scr_height);
			camera->fovy = 45.f;
			camera->updateProjMat();

			program = new Program("hatching prog");
			program->attatch("imnpr/hatching.vs").attatch("imnpr/hatching.fs").link();

			viewport = new Viewport(new MsFramebuffer());
			viewport->framebuffer->clear_color ={0.1f, 0.1f, 0.1f, 1.0f};

			sphere = MeshGenerator::genSphere(8, 5);
			//sphere = MeshGenerator::genQuad();

			models.push_back( ModelLoader::loadFile("common/archive/meshes/stanford-bunny.obj", true) );
			models.back()->position = glm::vec3(5, 0, 0);
			models.back()->scale = glm::vec3(50);
			models.back()->updateModelMat();
		}
		~AppHatching()
		{
			delete camera;
			delete program;
			delete viewport;
			for( Model* m : models ) {
				delete m;
			}
			delete sphere;
		}
	private:
		virtual void update() final
		{
			processInput(window);

			/* render to fbo in viewport */
			viewport->framebuffer->bind();

			GLuint pid = program->use();

			camera->aspect = viewport->framebuffer->aspect;
			camera->updateProjMat();
			setUniform(pid, "view", camera->view_mat);
			setUniform(pid, "camPos", camera->position);
			setUniform(pid, "projection", camera->proj_mat);

			glm::mat4 modelMat;

			modelMat = glm::translate(glm::mat4(1.f), glm::vec3(0.f));
			modelMat = glm::scale(modelMat, glm::vec3(1.f));
			setUniform(pid, "model", modelMat);
			sphere->draw();

			for( Model* m : models ) {
				setUniform(pid, "model", m->model_mat);
				m->meshes.back()->draw();
			}

			viewport->framebuffer->unbind();

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

		}
		virtual void renderImGui() final
		{
			imgui_modules::ShowExampleAppDockSpace([]() {});


			viewport->drawImGui();
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
