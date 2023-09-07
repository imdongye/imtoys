//
//  font rendering tester
//	2023-3-9 / im dong ye
//

#ifndef __app_font_h_
#define __app_font_h_

#include <limbrary/application.h>
#include <nanovg.h>
#include <imgui.h>

namespace lim
{
	class AppFont: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "font";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DISC = "font rendering tester with nanovg";
	private:
		std::string vert_shader;
		NVGcontext* vg = NULL;
		int fontNormal, fontBold, fontIcons, fontEmoji;
		ImFont* font1, *font0;
	public:
		AppFont();
		~AppFont();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif