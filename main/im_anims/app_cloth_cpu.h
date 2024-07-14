/*
	cloth simulation with particle mass system in cpu
	2024-07-01 / im dong ye
*/

#ifndef __app_particle_h_
#define __app_particle_h_

#include <limbrary/application.h>
#include "appbase_canvas3d.h"

namespace lim
{
	class AppClothCPU : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "particle";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";

	public:
		AppClothCPU();
		~AppClothCPU();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() const final;
		virtual void canvasImGui() final;
	};
}

#endif
