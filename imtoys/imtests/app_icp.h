//
//  framework template
//	2022-11-14 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef APP_TEMP_H
#define APP_TEMP_H

#include <limbrary/limclude.h>

namespace lim
{
	class AppICP: public AppBase
	{
	public:
		inline static constexpr const char const *APP_NAME = "test icp";
		inline static constexpr const char const *APP_DIR = "imtests/";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:
		AutoCamera *camera;
		Program *prog;
		Viewport *viewport;
		Mesh *sphere;
		Model *model;

	public:
		AppICP(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			prog = new Program("pbr", APP_DIR);
			prog->attatch("debug.vs").attatch("debug.fs").link();

			viewport = new Viewport(new MsFramebuffer());
			viewport->framebuffer->clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

			camera = new AutoCamera(window, viewport, 0, {0,1,5}, {0,1,0});

			model = ModelLoader::loadFile("common/archive/meshes/woody.obj", true);
			model->updateModelMat();
			for( Mesh* m :model->meshes )
				m->draw_mode = GL_POINTS;

			sphere = MeshGenerator::genSphere(50, 25);

			/* initialize static shader uniforms before rendering */
			GLuint pid = prog->use();
			setUniform(pid, "ao", 1.0f);
		}
		~AppICP()
		{
			delete camera;
			delete prog;
			delete viewport;
			delete model;
		}
	private:
		void update() override
		{
			viewport->framebuffer->bind();

			GLuint pid = prog->use();

			setUniform(pid, "viewMat", camera->view_mat);
			setUniform(pid, "cameraPos", camera->position);
			setUniform(pid, "projMat", camera->proj_mat);

			setUniform(pid, "modelMat", model->model_mat);
			for( Mesh* mesh: model->meshes ) {
				mesh->draw();
			}

			viewport->framebuffer->unbind();

			// clear backbuffer
			glEnable(GL_DEPTH_TEST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, fb_width, fb_height);
			glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		void renderImGui() override
		{
			ImGui::DockSpaceOverViewport();

			viewport->drawImGui();
			//ImGui::ShowDemoWindow();

			//ImGui::Begin("test fffafffff4f,window");
			//ImGui::End();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) override
		{
		}
		virtual void cursorPosCallback(double xPos, double yPos) override
		{
			static double xOld, yOld, xOff, yOff=0;
			xOff = xPos - xOld;
			yOff = yOld - yPos;

			xOld = xPos;
			yOld = yPos;
		}
	};
}

#endif
