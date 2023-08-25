//
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. 한글 지원
//

#ifndef __imgui_module_h_
#define __imgui_module_h_

#include <GLFW/glfw3.h>
#include <functional>

namespace lim
{
	class ImguiModule
	{
	public:
		inline static std::function<void()> draw_appselector = [](){};
	public:
		static void initImGui(GLFWwindow* window);
		static void beginImGui();
		static void endImGui(float frameWidth, float frameHeight);
		static void destroyImGui();
	};
}

#endif
