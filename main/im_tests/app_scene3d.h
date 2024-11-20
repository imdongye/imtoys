/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_scene3d_h_
#define __app_scene3d_h_

#include <limbrary/application.h>
#include <limbrary/3d/scene.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace lim
{
	class AppScene3d : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "3d scene editor";
		inline static constexpr CStr APP_DIR  = "im_tests/";
		inline static constexpr CStr APP_INFO = "3d scene editor";

		Scene scene;
		ViewportWithCam vp;

	public:
		AppScene3d();
		~AppScene3d();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
