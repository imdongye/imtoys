//
//  for test hdr image.
//	2022-11-14 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_SNELL_H
#define APP_SNELL_H

namespace lim
{
	class AppSnell: public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "snell'row tester";
		inline static constexpr const char *APP_DISC = "drag cursor to test";
	private:


	public:
		AppSnell(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);
		}
		~AppSnell()
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
			imgui_modules::ShowExampleAppDockSpace([]() {});

			ImGui::ShowDemoWindow();

			ImGui::Begin("state");
			//ImGui::PushItemWidth()
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
