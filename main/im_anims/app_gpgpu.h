/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_gpgpu_h_
#define __app_gpgpu_h_

#include <limbrary/application.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/transform.h>

namespace lim
{
	class AppGpgpu : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "gpgpu";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";


		Program prog;
		Program prog_xfb;
		MeshPlane ground;
		ViewportWithCamera viewport;

	public:
		AppGpgpu();
		~AppGpgpu();
		virtual void update() override;
		virtual void updateImGui() override;

		
	};
}

#endif
