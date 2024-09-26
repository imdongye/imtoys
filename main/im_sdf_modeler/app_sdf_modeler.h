/*
 	sdf modeler
	2023-2-28 / im dong ye
*/

#ifndef __app_sdf_modeler_h_
#define __app_sdf_modeler_h_

#include <limbrary/application.h>
#include <limbrary/model_view/viewport_with_cam.h>

namespace lim
{
	class AppSdfModeler: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "sdf_modeler";
		inline static constexpr CStr APP_DIR  = "im_sdf_modeler/";
		inline static constexpr CStr APP_INFO = "hello, world";

	private:
		ProgramReloadable prog;
		ViewportWithCam viewport;

	public:
		AppSdfModeler();
		~AppSdfModeler();
		
		virtual void update() final;
		virtual void updateImGui() final;
	};
}

#endif
