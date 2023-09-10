//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_model_viewer_h_
#define __app_model_viewer_h_

#include <limbrary/application.h>

namespace lim
{
	class AppModelViewer : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "model viewer";
		inline static constexpr CStr APP_DIR  = "im_model_viewer";
		inline static constexpr CStr APP_DISC = "model viewer for test materials";
	public:
		AppModelViewer();
		~AppModelViewer();
	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void cursorPosCallback(double xPos, double yPos) override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
