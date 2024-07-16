/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_cloth_gpu_
#define __app_cloth_gpu_

#include <limbrary/application.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/transform.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppClothGPU : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "cloth_gpu";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";


		Program prog;
		Program prog_xfb;
		MeshPlane ground;
		ViewportWithCamera viewport;
		Model model;

	public:
		AppClothGPU();
		~AppClothGPU();
		virtual void update() override;
		virtual void updateImGui() override;

		
	};
}

#endif
