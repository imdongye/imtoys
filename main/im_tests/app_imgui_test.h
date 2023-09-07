//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_imgui_test_h_
#define __app_imgui_test_h_

#include <limbrary/application.h>

namespace lim
{
	class AppImGuiTest : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "imgui_tester";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DISC = "limbrary imgui application";
	private:
	public:
		AppImGuiTest();
		~AppImGuiTest();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif
