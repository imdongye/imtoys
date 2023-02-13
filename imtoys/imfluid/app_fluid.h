//
//  framework template
//	2022-11-14 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef APP_FLUID_H
#define APP_FLUID_H

#include "../limbrary/limclude.h"
#include "canvas.h"

namespace lim
{
	class AppFluid: public AppBase
	{
	public:
		inline static constexpr const char *APP_DIR = "imfluid/";
		inline static constexpr const char *APP_NAME = "fluid tester";
		inline static constexpr const char *APP_DISC = "hello, world";
	private:
		Viewport* vp;
		CanvasGray* canvas;

	public:
		AppFluid(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			vp = new Viewport(new MsFramebuffer());
			vp->resizeCallback = [&](int w, int h) { this->vpResizeCallback(w, h); };

			canvas = new CanvasGray(vp->width,vp->height);
		}
		~AppFluid()
		{
			delete vp;
			delete canvas;
		}
	private:
		virtual void update() final
		{
			GrayToFramebuffer(canvas->tex_id, *vp->framebuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, scr_width, scr_height);
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

		}
		virtual void renderImGui() final
		{
			//imgui_modules::ShowExampleAppDockSpace([]() {});

			ImGui::Begin("state");
			ImGui::Text("%d %d", vp->mouse_pos.x, vp->mouse_pos.y);
			ImGui::End();

			vp->drawImGui();
		}
	public:
		void vpResizeCallback(int w, int h)
		{
			canvas->resize(w, h);
		}
	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		virtual void cursorPosCallback(double xPos, double yPos) final
		{
			int x = xPos;
			int y = yPos;
			if( !vp->dragging ) return;


			canvas->at(vp->mouse_pos.x, vp->mouse_pos.y) = 1.f;
			canvas->update();
		}
	};
}

#endif
