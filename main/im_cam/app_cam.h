/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_cam_h_
#define __app_cam_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace lim
{
	class AppCam : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "cam with cv";
		inline static constexpr CStr APP_DIR  = "im_cam/";
		inline static constexpr CStr APP_INFO = "import opencv test";

		Viewport viewport;

	public:
		AppCam();
		~AppCam();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
