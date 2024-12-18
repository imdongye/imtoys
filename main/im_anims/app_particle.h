/*
    2023-07-14 / im dong ye


*/
#ifndef __app_particle_h__
#define __app_particle_h__

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace lim
{
	class AppParticle : public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "particle system";
		inline static constexpr const char *APP_DIR  = "im_anims/";
		inline static constexpr const char *APP_INFO = "hello, world";

	private:
		ViewportWithCam viewport;

	public:
		AppParticle();
		~AppParticle();

	private:
		virtual void update() override;
		virtual void updateImGui() override;
    };
}

#endif