#include "app_moving_window.h"
#include <imgui.h>
#include <glad/glad.h>

namespace lim
{
	AppMovingWindow::AppMovingWindow(): AppBase(500, 500, APP_NAME)
	{
		int x, y;
		glfwGetWindowPos(window, &x, &y);
		win_pos = {x,y};
	}
	AppMovingWindow::~AppMovingWindow()
	{

	}
	void AppMovingWindow::update()
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
	void AppMovingWindow::updateImGui()
	{
		ImGui::Begin("test window");
		ImGui::Text("wasd to move window");
		ImGui::Text("window pos: %d %d", (int)win_pos.x, (int)win_pos.y);
		ImGui::End();
	}
	void AppMovingWindow::processInput()
	{
		const float speed = 50.f;
		glm::vec2 dir;
		dir.y = glfwGetKey(window, GLFW_KEY_S)-glfwGetKey(window, GLFW_KEY_W);
		dir.x = glfwGetKey(window, GLFW_KEY_D)-glfwGetKey(window, GLFW_KEY_A);

		win_pos += (float)(speed*delta_time)*dir;
	}
}
