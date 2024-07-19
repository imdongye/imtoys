/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_pbd_gpu_h_
#define __app_pbd_gpu_h_

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>

namespace lim
{
	class AppPbdGPU : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "PBD_GPU";
		inline static constexpr CStr APP_DIR  = "im_pbd";
		inline static constexpr CStr APP_DESCRIPTION = "pearlabyss summer internship";

		Program program;
		ViewportWithCamera viewport;

	public:
		AppPbdGPU();
		~AppPbdGPU();
		virtual void update() override;
		virtual void updateImGui() override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
