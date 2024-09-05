/*
	inverse kinematics with jacobi method
	2024-07-11 / imdongye

*/
#ifndef __app_ik_h_
#define __app_ik_h_

#include <limbrary/extension/appbase_canvas3d.h>

namespace lim
{
	class AppIK: public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "inverse kinematics";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_INFO = "hello, world";

	public:
		AppIK();
		~AppIK();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() const final;
		virtual void canvasImGui() final;
	};
}

#endif
