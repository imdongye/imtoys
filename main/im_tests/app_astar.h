//
//  for astar visualize
//	2023-03-03 / im dong ye
//
//	TODO list:
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
        inline static constexpr CStr APP_DIR  = "im_tests";
		inline static constexpr CStr APP_DESCRIPTION = "left click: destination, right click: start position";
	private:
		ImColor state_colors[7];
		int width, height;
		glm::ivec2 hover_pos;
	public:
		AppAstar();
		~AppAstar();
	private:
		virtual void update() final;
		void drawMap();
		void drawController();
		virtual void renderImGui() final;
	private:
		virtual void keyCallback(int key, int scancode, int action, int mods) final;
		virtual void cursorPosCallback(double xPos, double yPos) final;
		virtual void mouseBtnCallback(int button, int action, int mods) final;
	};
}

#endif
