/*
	imdongye
	fst 2024-11-06
	end 2024-11-06
*/

#ifndef __app_curvature_h_
#define __app_curvature_h_

#include <limbrary/application.h>
#include <limbrary/model_view/viewport_with_cam.h>
#include <limbrary/program.h>
#include <limbrary/model_view/mesh.h>
#include <limbrary/model_view/transform.h>

namespace lim
{
	class AppCurvature : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "curvature";
		inline static constexpr CStr APP_DIR  = "im_curvature/";
		inline static constexpr CStr APP_INFO = "high curvature group detection";

		Program program;
		OwnPtr<Mesh> mesh;
		Transform transform;
		ViewportWithCam viewport;

	public:
		AppCurvature();
		~AppCurvature();
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
