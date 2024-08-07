/*

2022-08-26 / im dong ye

Callbacks는 포인터를 키로 한 헤쉬테이블 맵이다. glfw콜백이 불렸을때 등록된 함수들을 실행해준다.
헤쉬테이블은 메모리 사용량이 많지만 프레임마다 전체탐색되기때문에 logN인 map(트리)대신 unordered_map을 사용했다.

실행순서: 생성자 -> {updateImGui, update} -> 소멸자

Todo:
1. crtp로 참조 줄이기, 
2. glfw에서 함수 이름 스네이크/카멜 혼용 이유 찾기
3. glfwSwapInterval(1); // vsync??

*/

#ifndef __application_h_
#define __application_h_

#include "g_tools.h"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>


namespace lim
{
	class AppBase
	{
	public:
		inline static std::function<void()> draw_appselector = [](){};
	public:
		GLFWwindow *window;
		
		float delta_time = 0.; // sec

		// relative to ratina or window monitor setting
		int win_width = 0;
		int win_height = 0;
		// real pixel coordinate
		int fb_width = 0;
		int fb_height = 0;
		float aspect_ratio = 1.f; // width/height;
		float pixel_ratio = 1.f;    // (DPI)
		int monitor_max_fps = 60;
		int max_fps = -1;
		glm::vec2 mouse_pos = {0,0};
		glm::vec2 mouse_off = {0,0};

		Callbacks<void(float deltaTime)> 							 update_hooks;
		Callbacks<void(int width, int height)> 						 win_size_callbacks;
		Callbacks<void(int width, int height)> 						 framebuffer_size_callbacks;
		Callbacks<void(int key, int scancode, int action, int mods)> key_callbacks;
		Callbacks<void(int button, int action, int mods)>			 mouse_btn_callbacks;
		Callbacks<void(double xOff, double yOff)>					 scroll_callbacks;
		Callbacks<void(double xPos, double yPos)>					 cursor_pos_callbacks;
		Callbacks<void(int count, const char **paths)>				 dnd_callbacks;

	public:
		AppBase(const AppBase&) = delete;
		AppBase(AppBase&&) = delete;
		AppBase& operator=(const AppBase&) = delete;
		AppBase& operator=(AppBase&&) = delete;
	
		/* init */
		AppBase(int winWidth=1280, int winHeight=720, const char* title="nonamed", bool vsync=true);
		/* destroy */
		virtual ~AppBase();
		void run();
	protected:
		virtual void update()=0;
		virtual void updateImGui()=0;
		#pragma warning( disable : 4100 )
		virtual void framebufferSizeCallback(int w, int h) {};
		virtual void keyCallback(int key, int scancode, int action, int mods) {};
		virtual void cursorPosCallback(double xPos, double yPos) {};
		virtual void mouseBtnCallback(int button, int action, int mods) {};
		virtual void scrollCallback(double xOff, double yOff) {};
		virtual void dndCallback(int count, const char **paths) {};
		#pragma warning( default : 4100 )
	};
}

#endif
