//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_model_viewer_h_
#define __app_model_viewer_h_

#include <limbrary/application.h>
#include <limbrary/model_view/camera_man.h>
#include <limbrary/program.h>
#include <limbrary/model_view/renderer.h>

namespace lim
{
	class AppModelViewer : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "model viewer";
		inline static constexpr CStr APP_DIR  = "im_model_viewer";
		inline static constexpr CStr APP_DESCRIPTION = "model viewer for test materials";
	private:
		std::vector<ViewportWithCamera*> viewports;
		Viewport vp_light_map, vp_irr_map, vp_pfenv_map, vp_pfbrdf_map;
		std::vector<Scene*> scenes;
		IBLight ib_light;
		Model floor_md;

		ProgramReloadable program;

	public:
		AppModelViewer();
		~AppModelViewer();
	private:
		void addModelViewer(std::string path);
		void rmModelViewer(int idx);
		void drawModelsToViewports();

		virtual void update() override;
		virtual void updateImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
