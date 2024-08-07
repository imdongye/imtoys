/*

2022-08-24 / im dong ye

imgui framebuffer viewer

Note:
* Framebuffer은 Dependent Injection으로 외부에서 생성돼서 들어오지만 
  viewport에 종속돼서 viewport와 같이 삭제된다.
* dragging with left or middle button


Todo:
0. template화하는게 더 좋을까?(부모클래스 탬플릿조건)
1. initial framebuffer size setting
2. drag imgui demo 참고해서 다시짜기
3. https://github.com/ocornut/imgui/issues/3152
	https://github.com/ocornut/imgui/issues/3492
	https://jamssoft.tistory.com/234
	https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
	https://github.com/ocornut/imgui/issues/270
	로 앱별 tag부여해서 ini 윈도우 설정 겹치지 않게

*/

#ifndef __viewport_h_
#define __viewport_h_

#include <glad/glad.h>
#include "framebuffer.h"
#include <glm/glm.hpp>
#include <string>
#include "g_tools.h"


namespace lim
{
	class Camera;

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
		std::string name = "nonamed";
		WindowMode window_mode = WM_FREE;
		bool is_opened = true;
		bool is_hovered = false;
		bool is_focused = false;
		bool is_dragged = false;
		bool is_scrolled = false;
		bool is_appearing = false;
		glm::vec2 mouse_pos = {0,0};
		glm::vec2 mouse_uv_pos = {0,0};
		glm::vec2 mouse_off = {0,0};
		glm::vec2 scroll_off = {0,0};
		Callbacks<void(int, int)> resize_callbacks;
		Callbacks<void(float dt)> update_callbacks;
	protected:
		glm::vec2 prev_mouse_pos = {0,0};
		IFramebuffer* own_framebuffer = nullptr;
	public:
		Viewport(const Viewport&) = delete;
		Viewport(Viewport&&) = delete;
		Viewport& operator=(const Viewport&) = delete;
		Viewport& operator=(Viewport&&) = delete;
		
		Viewport(std::string_view _name, IFramebuffer* createdFB);
		virtual ~Viewport();

		bool drawImGui(std::function<void(const Viewport&)> guizmoFunc = nullptr);
		void resize(GLuint _width, GLuint _height);
		const IFramebuffer& getFb();
		void setClearColor(glm::vec4 color);
		int getWidth() const;
		int getHeight() const;
		float getAspect() const;
	};
}

#endif