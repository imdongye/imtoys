/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_particle_h_
#define __app_particle_h_

#include <limbrary/application.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/model_view/camera_man.h>
#include "canvas3d_app.h"

namespace lim
{
	class AppParticle : public Canvas3dApp
	{
	public:
		inline static constexpr CStr APP_NAME = "particle";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "limbrary template application";

	public:
		AppParticle();
		~AppParticle();
		virtual void render() override;
		virtual void renderImGui() override;
		
	};
}

#endif
