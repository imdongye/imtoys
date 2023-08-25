//
//  framework template
//	2022-11-14 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef __app_template_h_
#define __app_template_h_

#include <limbrary/application.h>

namespace lim
{
	class AppTemplate : public AppBase
	{
	public:
		inline static constexpr const char *const APP_NAME = "template";
		inline static constexpr const char *const APP_DIR  = "imtests/";
		inline static constexpr const char *const APP_DISC = "limbrary template application";
	private:
	public:
		AppTemplate();
		~AppTemplate();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
	};
}

#endif
