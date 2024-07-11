/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_particle_h_
#define __app_particle_h_

#include <limbrary/application.h>
#include "appbase_canvas3d.h"

namespace lim
{
	class AppParticle : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "particle";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";

	public:
		AppParticle();
		~AppParticle();
		virtual void canvasUpdate() final;
		virtual void canvasDraw() final;
		virtual void canvasImGui() final;
	};
}

#endif
