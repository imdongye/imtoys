//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
// 
//	callback을 window data에 직접 추가할수도있고 가상함수는 기본으로 등록되어있어서 오버라이딩하기만 하면됨.
//	상속한 앱에서 등록하는 코드가 없어서 깔끔함. 하지만 가상함수테이블에서 포인팅되는과정이 성능에 문제될수있다.
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. glfw에서 함수 이름 스네이크/카멜 혼용 이유 찾기
//  3. glfwSwapInterval(1); // vsync??
//

#ifndef __application_h_
#define __application_h_

#include "utils.h"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>


namespace lim
{
	class AppBase
	{
	public:
		GLFWwindow *window;
		
		Callbacks<void(int width, int height)> win_size_callbacks;
		Callbacks<void(int width, int height)> framebuffer_size_callbacks;
		Callbacks<void(int key, int scancode, int action, int mods)> key_callbacks;
		Callbacks<void(int button, int action, int mods)> mouse_btn_callbacks;
		Callbacks<void(double xOff, double yOff)> scroll_callbacks;
		Callbacks<void(double xPos, double yPos)> cursor_pos_callbacks;
		Callbacks<void(int count, const char **paths)> dnd_callbacks;
		Callbacks<void(float deltaTime)> update_hooks;
		
		float delta_time; // sec

		// relative to ratina or window monitor setting
		int win_width, win_height;
		// real pixel coordinate
		int fb_width, fb_height;
		float aspect_ratio; // width/height;
		float pixel_ratio;    // (DPI)
		glm::vec2 mouse_pos;

	protected:
		virtual void update()=0;
		virtual void renderImGui()=0;
		virtual void framebufferSizeCallback(int w, int h) {};
		virtual void keyCallback(int key, int scancode, int action, int mods) {};
		virtual void cursorPosCallback(double xPos, double yPos) {};
		virtual void mouseBtnCallback(int button, int action, int mods) {};
		virtual void scrollCallback(double xOff, double yOff) {};
		virtual void dndCallback(int count, const char **paths) {};
	public:
		/* init */
		AppBase(int winWidth=1280, int winHeight=720, const char* title="nonamed");
		/* destroy */
		virtual ~AppBase();
		void run();
	private:
		void initGlfwCallbacks();

		void printVersionAndStatus();
	};
}

#endif
