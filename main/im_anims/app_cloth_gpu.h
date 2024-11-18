/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_cloth_gpu_
#define __app_cloth_gpu_

#include <limbrary/application.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/transform.h>
#include <limbrary/3d/model.h>
#include <limbrary/3d/light.h>

namespace lim
{
	class AppClothGPU : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "cloth_gpu";
		inline static constexpr CStr APP_DIR  = "im_anims/";
		inline static constexpr CStr APP_INFO = "limbrary template application";


		Program prog_render;
		Program prog_skin;
		Program prog_comp;
		Program prog_comp_nor;
		MeshPlane ground;
		ViewportWithCam viewport;
		Model model;
		LightDirectional light;

	public:
		AppClothGPU();
		~AppClothGPU();
		virtual void update() override;
		virtual void updateImGui() override;

		
	};
}

#endif
