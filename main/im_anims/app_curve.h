/*
    2023-07-14 / im dong ye


*/
#ifndef __app_curve_h__
#define __app_curve_h__

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>

namespace lim
{
	class AppCurve : public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "curve interpolation";
		inline static constexpr const char *APP_DIR  = "im_anims";
		inline static constexpr const char *APP_DESCRIPTION = "hello, world";

	public:
		AppCurve();
		~AppCurve();

	private:
		virtual void update() override;
		virtual void updateImGui() override;
    };
}

#endif