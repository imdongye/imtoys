/*

2022-08-24 / im dong ye


Note:
	viewport is additional information for framebuffer,

	and can viewing framebuffer with imgui.

	constructor have dependent injection with framebuffer.

	you can get framebuffer information with getFb()
		ex) getFb().aspect

Ref:
	https://github.com/ocornut/imgui/issues/3152
	https://github.com/ocornut/imgui/issues/3492
	https://jamssoft.tistory.com/234
	https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
	https://github.com/ocornut/imgui/issues/270

*/

#ifndef __viewport_h_
#define __viewport_h_

#include <glad/glad.h>
#include "framebuffer.h"
#include <glm/glm.hpp>
#include <string>
#include "containers/callbacks.h"
#include "containers/own_ptr.h"
#include "tools/mecro.h"


namespace lim
{
	class Viewport : public NoCopyAndMove
	{
	public:
		enum WindowMode : int
		{
			WM_FREE,
			WM_FIXED_RATIO,
			WM_FIXED_SIZE,
		};
		WindowMode window_mode = WM_FREE;
		float fixed_aspect = 1.0f; // for WM_FIXED_RATIO

		std::string name = "Viewport##appname";
		bool is_opened = true;
		bool is_hidden = false;
		bool is_hovered = false;
		bool is_focused = false;
		bool is_dragged = false;
		bool is_size_changed = true;

		glm::ivec2 fb_size;

		// for guizmo
		glm::vec2 	content_pos;
		glm::vec2 	content_size;

		// screen space
		glm::vec2 	mouse_pos = {0,0}; 
		glm::vec2 	mouse_uv_pos = {0,0};

	protected:
		OwnPtr<IFramebuffer> own_framebuffer = nullptr;

	public:
		Viewport(IFramebuffer* createdFB, const char* _name = "Viewport");
		virtual ~Viewport() noexcept = default;

		void drawImGui();

		inline void resize(const glm::ivec2& _size) {
			fixed_aspect = float(_size.x) / float(_size.y);
			fb_size = _size;
			content_size = glm::vec2(_size);
			own_framebuffer->resize(_size);
		}

		inline IFramebuffer& getFb() {
			return *own_framebuffer;
		}
	};
}

#endif