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
		std::string name = "nonamed";
		WindowMode window_mode = WM_FREE;
		bool window_opened = true;
		bool hovered = false;
		bool focused = false;
		bool dragging = false;
		glm::vec2 mouse_pos = {0,0};
		bool is_scrolled = false;
		glm::vec2 scroll_off = {0,0};
		Callbacks<void(int, int)> resize_callbacks;
		Callbacks<void(float dt)> update_callbacks;
	private:
		Framebuffer* framebuffer = nullptr;
		Viewport(const Viewport&) = delete;
		Viewport& operator=(const Viewport&) = delete;
	public:
		Viewport(std::string_view _name, Framebuffer* createdFB);
		Viewport(Viewport&& src) noexcept;
		Viewport& operator=(Viewport&& src) noexcept;
		virtual ~Viewport() noexcept;

		bool drawImGui(); // and resize fb and return windowOpened
		void resize(GLuint _width, GLuint _height);
		const Framebuffer& getFb();
		void setClearColor(glm::vec4 color);
		const GLuint& getWidth() const;
		const GLuint& getHeight() const;
		const float& getAspect() const;
	};
}

#endif