/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_template_h_
#define __app_template_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace lim
{
	class AppTemplate : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "template";
		inline static constexpr CStr APP_DIR  = "im_tests/";
		inline static constexpr CStr APP_INFO = "limbrary template application";

		Viewport viewport;

	public:
		AppTemplate();
		~AppTemplate();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
