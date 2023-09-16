//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_template_h_
#define __app_template_h_

#include <limbrary/application.h>

namespace lim
{
	class AppTemplate : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "template";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DISC = "limbrary template application";
	private:
	public:
		AppTemplate();
		~AppTemplate();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif