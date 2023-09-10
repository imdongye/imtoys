#include "app_template.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <imgui_internal.h>

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

		log::drawViewer("logger##template");;
		

		ImGui::Begin("test window##template");
		ImGui::Text("win size : %d %d", win_width, win_height);
		ImGui::Text("fb size  : %d %d", fb_width, fb_height);
		ImGui::End();
	}
	void AppTemplate::keyCallback(int key, int scancode, int action, int mods)
	{
		log::pure("%d\n", key);
		log::pure("%d\n", key);
		log::warn("%d\n", key);
		log::err("%d\n", key);
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