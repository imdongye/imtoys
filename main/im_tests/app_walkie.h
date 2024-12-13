/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_walkie_h_
#define __app_walkie_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace lim
{
	class AppWalkie : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "walkie talk";
		inline static constexpr CStr APP_DIR  = "im_tests/";
		inline static constexpr CStr APP_INFO = "limbrary template application";

	public:
		AppWalkie();
		~AppWalkie();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
