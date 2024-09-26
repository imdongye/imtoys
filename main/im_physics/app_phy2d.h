/*
    2023-08-12 / im dong ye


*/
#ifndef __app_phy2d_h__
#define __app_phy2d_h__

#include <limbrary/application.h>
#include <limbrary/model_view/viewport_with_cam.h>

namespace lim
{
	class AppPhy2d : public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "physics2d test";
		inline static constexpr const char *APP_DIR  = "im_physics/";
		inline static constexpr const char *APP_INFO = "hello, world";

	public:
		AppPhy2d();
		~AppPhy2d();

	private:
		virtual void update() override;
		virtual void updateImGui() override;
    };
}

#endif