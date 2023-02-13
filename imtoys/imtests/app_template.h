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

#include "../limbrary/limclude.h"

namespace lim
{
	class AppTemplate: public AppBase
	{
	public:
        inline static constexpr const char const *APP_DIR = "imtests/";
        inline static constexpr const char const *APP_NAME = "template";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:


	public:
		AppTemplate(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);
		}
		~AppTemplate()
		{

		}
	private:
		virtual void update() final
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

		}
		virtual void renderImGui() final
		{
			//imgui_modules::ShowExampleAppDockSpace([]() {});

			ImGui::ShowDemoWindow();

			ImGui::Begin("test window");
			ImGui::End();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		virtual void cursorPosCallback(double xPos, double yPos) final
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
