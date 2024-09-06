/*
	softbody in model final project
	2022-11-14 / im dong ye
*/

#ifndef __app_softbody_in_model_h_
#define __app_softbody_in_model_h_

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/scene.h>
#include "pbd/pbd.h"

namespace lim
{
	class AppSoftbodyInModel : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "Softbody In Model";
		inline static constexpr CStr APP_DIR  = "im_pbd/";
		inline static constexpr CStr APP_INFO = "in pearabyss";

		Program prog_skinned;
		Program prog_static;
		ViewportWithCamera viewport;


		IBLight ib_light;
		Scene scene;
		pbd::PhySceneGpu phy_scene;

		
		void reloadModel(const char* path);

	public:
		AppSoftbodyInModel();
		virtual ~AppSoftbodyInModel();
		virtual void update() override;
		virtual void updateImGui() override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
