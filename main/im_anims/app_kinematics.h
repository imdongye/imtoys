//
//  forward backword kinematics
//	2023-2-28 / im dong ye
//

#ifndef __app_kinematics_h_
#define __app_kinematics_h_

#include <limbrary/application.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/model_view/camera_man.h>

namespace lim
{
	class AppKinematics: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "imkenematics";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "hello, world";
	private:
		Scene scene;
		ViewportWithCamera viewport;

	public:
		AppKinematics();
		~AppKinematics();
	private:
		virtual void update() final;
		virtual void updateImGui() final;
	};
}

#endif
