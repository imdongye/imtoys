/*
	inverse kinematics with jacobi method
	2024-07-11 / imdongye

*/
#ifndef __app_kinematics_h_
#define __app_kinematics_h_

#include <limbrary/application.h>
#include "appbase_canvas3d.h"

namespace lim
{
	class AppIK: public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "imkenematics";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "hello, world";

	public:
		AppIK();
		~AppIK();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() const final;
		virtual void canvasImGui() final;
	};
}

#endif
