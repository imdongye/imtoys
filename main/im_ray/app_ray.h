 /*
	2023-08-03 / imdongye


*/

#ifndef __app_ray_h_
#define __app_ray_h_

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/program.h>
#include <limbrary/model_view/model.h>

namespace lim
{
	class AppRay: public AppBase
	{
	public:
        inline static constexpr CStr APP_NAME = "ray tracer";
		inline static constexpr CStr APP_DIR  = "im_ray";
		inline static constexpr CStr APP_DESCRIPTION = "compute shader ray tracer";
	private:
		Program prog;
        ViewportWithCamera viewport;

	public:
		AppRay();
		~AppRay();
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
