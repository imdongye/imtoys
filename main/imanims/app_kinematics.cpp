#include "app_kinematics.h"
#include <imgui.h>
#include <stb_image.h>
#include <glad/glad.h>

namespace lim
{
	AppKinematics::AppKinematics(): AppBase(600, 600, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
		int x, y;
		glfwGetWindowPos(window, &x, &y);
		win_pos = {x,y};
	}
	AppKinematics::~AppKinematics()
	{

	}
	void AppKinematics::update()
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
	void AppKinematics::renderImGui()
	{
		//ImGui::DockSpaceOverViewport();

		//ImGui::ShowDemoWindow();

		ImGui::Begin("test window");
		ImGui::End();
	}
	void AppKinematics::processInput()
	{
		const float speed = 50.f;
		glm::vec2 dir;
		dir.y = glfwGetKey(window, GLFW_KEY_S)-glfwGetKey(window, GLFW_KEY_W);
		dir.x = glfwGetKey(window, GLFW_KEY_D)-glfwGetKey(window, GLFW_KEY_A);

		win_pos += (float)(speed*delta_time)*dir;
	}
}
