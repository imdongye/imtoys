/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_skeletal_h_
#define __app_skeletal_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/scene.h>

namespace lim
{
	class AppSkeletal : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "Skeletal Animation";
		inline static constexpr CStr APP_DIR  = "im_anims/";
		inline static constexpr CStr APP_INFO = "learn with LearnOpenGL";

		Program prog_skinned;
		Program prog_static;
		ViewportWithCam viewport;
		Scene scene;
		void makeScene(const char* path);

	public:
		AppSkeletal();
		~AppSkeletal();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
