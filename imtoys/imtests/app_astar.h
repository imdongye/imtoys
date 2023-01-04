//
//  for astar visualize
//	2023-03-03 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_ASTAR_H
#define APP_ASTAR_H

namespace lim
{
	class AppAstar: public AppBase
	{
	public:
        inline static constexpr const char *APP_DIR = "imtests/";
        inline static constexpr const char *APP_NAME = "astar visualizer";
		inline static constexpr const char *APP_DISC = "left click: destination, right click: start position";
	private:


	public:
		AppAstar(): AppBase(720, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);
		}
		~AppAstar()
		{

		}
		void drawMap()
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			ImGui::PushItemWidth(-ImGui::GetFontSize() * 15);
			ImGui::Begin("map");
			
			const ImVec2 p = ImGui::GetCursorScreenPos();
			const float sz = 32.f;
			const float pd = 2.f;
			const ImU32 color = ImColor(1.0f, 0.0f, 0.0f);
			float x = p.x;
			float y = p.y;

			for( int i=0; i<10; i++ ) {
				for( int j=0; j<10; j++ ) {
					drawList->AddRect({x, y}, {x+sz, y+sz}, color, sz/5.0f);
					x += sz+pd;
				}
				y += sz+pd;
			}

			ImGui::End();
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

			//ImGui::ShowDemoWindow();
			
			//drawMap();
		}

	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
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
