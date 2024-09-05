//
//  framework template
//	2022-11-14 / im dong ye
//

#ifndef __app_moving_window_h_
#define __app_moving_window_h_

#include <limbrary/application.h>

namespace lim
{
	class AppMovingWindow : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME = "moving window";
		inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_INFO = "hi";
	private:
		glm::vec2 win_pos;
	public:
		AppMovingWindow();
		~AppMovingWindow();
	private:
		virtual void update() override;
		virtual void updateImGui() override;
		void processInput();
	};
}

#endif
