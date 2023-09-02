#include "app_template.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/logger.h>
#include <imgui.h>

namespace lim
{
	AppTemplate::AppTemplate() : AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
	}
	AppTemplate::~AppTemplate()
	{
	}
	void AppTemplate::update() 	{
		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void AppTemplate::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		ImGui::Begin("test window##template");
		ImGui::Text("%d", 1);
		ImGui::End();
	}
	void AppTemplate::keyCallback(int key, int scancode, int action, int mods)
	{
		Logger::get() << ImGui::GetFrameHeight();
	}
	void AppTemplate::cursorPosCallback(double xPos, double yPos)
	{
		static double xOld, yOld, xOff, yOff = 0;
		xOff = xPos - xOld;
		yOff = yOld - yPos;

		xOld = xPos;
		yOld = yPos;
	}
}