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
	class AppPbdCpu : public AppBaseCanvas3d
	{
	public:
		inline static constexpr CStr APP_NAME = "PBD_CPU";
		inline static constexpr CStr APP_DIR  = "im_pbd";
		inline static constexpr CStr APP_DESCRIPTION = "pearlabyss summer internship";
		Program prog_ms;
		
	public:
		AppPbdCpu();
		~AppPbdCpu();
		
        virtual void customDrawShadow( const glm::mat4& mtx_View, const glm::mat4& mtx_Proj ) const final;
		virtual void customDraw( const Camera& cam, const LightDirectional& lit ) const final;
		virtual void canvasUpdate() final;
        virtual void canvasDraw() const final;
        virtual void canvasImGui() final;
	};
}

#endif
