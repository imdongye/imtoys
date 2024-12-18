 /*
	2023-08-03 / imdongye


*/

#ifndef __app_ray_h_
#define __app_ray_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/program.h>
#include <limbrary/3d/model.h>

namespace lim
{
	class AppRay: public AppBase
	{
	public:
        inline static constexpr CStr APP_NAME = "ray tracer";
		inline static constexpr CStr APP_DIR  = "im_ray/";
		inline static constexpr CStr APP_INFO = "compute shader ray tracer";
	private:
		Program prog;
        ViewportWithCam viewport;

	public:
		AppRay();
		~AppRay();
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
