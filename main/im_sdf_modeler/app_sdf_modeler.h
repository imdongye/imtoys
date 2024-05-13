/*
 	sdf modeler
	2023-2-28 / im dong ye
*/

#ifndef __app_sdf_modeler_h_
#define __app_sdf_modeler_h_

#include <limbrary/application.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/model_view/camera_man.h>

namespace lim
{
	class AppSdfModeler: public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "sdf_modeler";
		inline static constexpr CStr APP_DIR  = "im_sdf_modeler";
		inline static constexpr CStr APP_DESCRIPTION = "hello, world";

	private:
		ProgramReloadable prog;
		ViewportWithCamera viewport;

	public:
		AppSdfModeler();
		~AppSdfModeler();
		
		virtual void update() final;
		virtual void updateImGui() final;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void dndCallback(int count, const char **paths) override;
		virtual void mouseBtnCallback(int button, int action, int mods) override;
	};
}

#endif
