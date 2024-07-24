/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_pbd_cpu_h_
#define __app_pbd_cpu_h_

#include <limbrary/extension/appbase_canvas3d.h>
#include "pbd/pbd.h"

namespace lim
{
	class AppPbdCPU : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "PBD_CPU";
		inline static constexpr CStr APP_DIR  = "im_pbd";
		inline static constexpr CStr APP_DESCRIPTION = "pearlabyss summer internship";

	public:
		AppPbdCPU();
		~AppPbdCPU();
		
		virtual void canvasUpdate() final;
        virtual void canvasDraw() const final;
        virtual void canvasImGui() final;
	};
}

#endif
