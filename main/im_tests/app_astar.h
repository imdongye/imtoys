//
//  for astar visualize
//	2023-03-03 / im dong ye
//
//	Todo:
//
//

#ifndef __app_astar_h_
#define __app_astar_h_

#include <limbrary/application.h>
#include <imgui.h>

namespace lim
{
	class AppAstar: public AppBase
	{
	public:
        inline static constexpr CStr APP_NAME = "astar visualizer";
        inline static constexpr CStr APP_DIR  = "im_tests/";
		inline static constexpr CStr APP_INFO = "left click: destination, right click: start position";
	private:
		ImColor state_colors[7];
		int width, height;
		glm::ivec2 hover_pos;
	public:
		AppAstar();
		~AppAstar();
	private:
		void drawMap();
		void drawController();
		virtual void updateImGui() final;
		virtual void update() final;
	};
}

#endif
