
//
//	2022-08-24 / im dong ye
//
//	TODO list:
//	1. initial framebuffer size setting
//	2. drag imgui demo 참고해서 다시짜기
//  3. https://github.com/ocornut/imgui/issues/3152
//		https://github.com/ocornut/imgui/issues/3492
//		https://jamssoft.tistory.com/234
//		https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
//		https://github.com/ocornut/imgui/issues/270
//		로 앱별 tag부여해서 ini 윈도우 설정 겹치지 않게

#ifndef __viewport_h_
#define __viewport_h_

#include <glad/glad.h>
#include "framebuffer.h"
#include <glm/glm.hpp>
#include <string>
#include "utils.h"


namespace lim
{
	class Viewport
	{
	public:
		enum WindowMode
		{
			WM_FREE,
			WM_FIXED_RATIO,
			WM_FIXED_SIZE,
		};
	public:
		/* c++17 non-const static data member can initialize in declaration with inline keyword*/
		inline static GLuint id_generator = 0;
		GLuint id;
		std::string name;
		Framebuffer* framebuffer;
		bool hovered, focused, dragging;
		GLuint width, height;
        float aspect;
		glm::ivec2 mouse_pos;
		WindowMode window_mode;
		bool window_opened;
		Callbacks<void(int, int)> resize_callbacks;
	public:
		Viewport(Framebuffer* createdFB, GLuint _width=256, GLuint _height=256, WindowMode wm=WM_FREE);
		virtual ~Viewport();
		void drawImGui();
	};
}

#endif