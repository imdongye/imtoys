/*
    2023-07-14 / im dong ye


*/
#ifndef __app_particle_h__
#define __app_particle_h__

#include <limbrary/application.h>
#include <limbrary/viewport.h>

namespace lim
{
	class AppParticle : public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "particle system";
		inline static constexpr const char *APP_DIR  = "im_anims";
		inline static constexpr const char *APP_DESCRIPTION = "hello, world";

	private:
		Viewport viewport;

	public:
		AppParticle();
		~AppParticle();

	private:
		virtual void update() override;
		virtual void updateImGui() override;
    };
}

#endif