/*
	framework template
	2022-11-14 / im dong ye
*/

#ifndef __app_skeletal_h_
#define __app_skeletal_h_

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/model_view/animator.h>

namespace lim
{
	class AppSkeletal : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "Skeletal Animation";
		inline static constexpr CStr APP_DIR  = "im_anims";
		inline static constexpr CStr APP_DESCRIPTION = "learn with LearnOpenGL";

		Program program;
		ViewportWithCamera viewport;
		Scene scene;
		Model model;
		RdNode* cur_nd = nullptr;
		
		void drawHierarchy(RdNode& nd);
		void drawInspector(RdNode& nd);
		void importModel(const char* path);

	public:
		AppSkeletal();
		~AppSkeletal();
		virtual void update() override;
		virtual void updateImGui() override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
