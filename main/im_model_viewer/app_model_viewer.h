//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_model_viewer_h_
#define __app_model_viewer_h_

#include <limbrary/application.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/program.h>
#include <limbrary/3d/scene.h>

namespace lim
{
	class AppModelViewer : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "model viewer";
		inline static constexpr CStr APP_DIR  = "im_model_viewer/";
		inline static constexpr CStr APP_INFO = "model viewer for test materials";
	private:
		std::vector<ViewportWithCam*> viewports;
		Viewport vp_light_map, vp_irr_map, vp_pfenv_map, vp_pfbrdf_map;
		std::vector<Scene*> scenes;
		IBLight ib_light;
		Model floor_md;

		ProgramReloadable program;

	public:
		AppModelViewer();
		~AppModelViewer();
	private:
		void addModelViewer(const char* path);
		void rmModelViewer(int idx);
		void drawModelsToViewports();

		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
