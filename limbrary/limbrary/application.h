/*
	imdongye@naver.com
	fst: 2022-08-26
	lst: 2022-08-26

Note:
	lifecycle of application
		1. constructor
		2. loop
			a. updateImGui
			b. update
		3. destructor
Todo:
	#pragma once for clang, etc...

*/

#ifndef __application_h_
#define __application_h_

#include "containers/callbacks.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "containers/types.h"
#include "tools/mecro.h"


namespace lim
{
	class AppBase : public NoCopyAndMove
	{
	public:
		inline static AppBase* 	  g_ptr = nullptr;
		inline static const char* g_app_name = nullptr;
		inline static const char* g_app_dir = nullptr;
		inline static const char* g_app_info = nullptr;

	public:
		GLFWwindow* window;
		
		float delta_time = 0.f; // sec

		int monitor_max_fps;
		int custom_max_fps = -1; // -1 = unlimited

		bool is_focused = true;
		bool is_size_changed = true;
		int win_width = 0; // relative to ratina or window monitor setting
		int win_height = 0;		
		int fb_width = 0; // real pixel coordinate
		int fb_height = 0;
		float aspect_ratio = 1.f; // width/height;
		float pixel_ratio = 1.f; // (DPI)


		Callbacks<void(float deltaTime)> 							 update_hooks;
		Callbacks<void(int width, int height)> 						 framebuffer_size_callbacks;
		Callbacks<void(int width, int height)> 						 win_size_callbacks;
		Callbacks<void(int key, int scancode, int action, int mods)> key_callbacks;
		Callbacks<void(int button, int action, int mods)>			 mouse_btn_callbacks;
		Callbacks<void(glm::vec2 pos)>					 	 		 cursor_pos_callbacks;
		Callbacks<void(glm::vec2 off)>					 	 		 scroll_callbacks;
		Callbacks<void(int count, const char **paths)>				 dnd_callbacks;

	public:
		AppBase(int winWidth=1280, int winHeight=720, const char* title="nonamed", bool vsync=true);
		virtual ~AppBase();
		void run();

	protected:
		virtual void update()=0;
		virtual void updateImGui()=0;
	};
}

#endif
