/*
	cloth simulation with particle mass system in cpu
	2024-07-01 / im dong ye
*/

#ifndef __app_cloth_cpu_h_
#define __app_cloth_cpu_h_

#include <limbrary/extension/appbase_canvas3d.h>

namespace lim
{
	class AppClothCPU : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "cloth_cpu";
		inline static constexpr CStr APP_DIR  = "im_anims/";
		inline static constexpr CStr APP_INFO = "limbrary template application";

	public:
		AppClothCPU();
		~AppClothCPU();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() const final;
		virtual void canvasImGui() final;
	};
}

#endif
