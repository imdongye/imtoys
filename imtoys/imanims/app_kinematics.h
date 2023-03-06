//
//  forward backword kinematics
//	2023-2-28 / im dong ye
//

#ifndef APP_KINEMATICS_H
#define APP_KINEMATICS_H

#include "../limbrary/limclude.h"

namespace lim
{
	class AppKinematics: public AppBase
	{
	public:
		inline static constexpr const char const *APP_NAME = "imkenematics";
		inline static constexpr const char const *APP_DIR = "imanims/";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:
		glm::vec2 win_pos;

	public:
		AppKinematics(): AppBase(600, 600, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);
			int x, y;
			glfwGetWindowPos(window, &x, &y);
			win_pos = {x,y};
		}
		~AppKinematics()
		{

		}
	private:
		virtual void update() final
		{

			glfwSetWindowPos(window, win_pos.x, win_pos.y);

			// clear backbuffer
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, fb_width, fb_height);
			glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			processInput();
		}
		virtual void renderImGui() final
		{
			//ImGui::DockSpaceOverViewport();

			//ImGui::ShowDemoWindow();

			ImGui::Begin("test window");
			ImGui::End();
		}

	private:
		void processInput()
		{
			const float speed = 50.f;
			glm::vec2 dir;
			dir.y = glfwGetKey(window, GLFW_KEY_S)-glfwGetKey(window, GLFW_KEY_W);
			dir.x = glfwGetKey(window, GLFW_KEY_D)-glfwGetKey(window, GLFW_KEY_A);

			win_pos += (float)(speed*delta_time)*dir;
		}
	};
}

#endif
