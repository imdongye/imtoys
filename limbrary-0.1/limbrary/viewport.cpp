#include <limbrary/viewport.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <limbrary/model_view/camera.h>
#include <imguizmo/ImGuizmo.h>

namespace lim 
{
	Viewport::Viewport(std::string_view _name, IFramebuffer* createdFB)
		:name(_name)
	{
		if( createdFB==nullptr )
			return;
		framebuffer = createdFB;
		framebuffer->resize(256, 256); // default size
	}
	Viewport::Viewport(Viewport&& src) noexcept
	{
		*this = std::move(src);
	}
	Viewport& Viewport::operator=(Viewport&& src) noexcept
	{
		if(this != &src) {
			name 			 = std::move(src.name);
			window_mode 	 = src.window_mode;
			is_opened	 = src.is_opened;
			is_hovered 		 = src.is_hovered;
			is_focused 		 = src.is_focused;
			is_dragged 		 = src.is_dragged;
			mouse_pos 		 = src.mouse_pos;
			is_scrolled = src.is_scrolled;
			scroll_off  = src.scroll_off;

			resize_callbacks = std::move(src.resize_callbacks);
			update_callbacks = std::move(src.update_callbacks);

			if( framebuffer )
				delete framebuffer;
			framebuffer = src.framebuffer;
			src.framebuffer = nullptr;
		}
		return *this; 
	}
	Viewport::~Viewport() noexcept
	{
		if( framebuffer )
			delete framebuffer;
	}
	bool Viewport::drawImGui(std::function<void(const Viewport&)> guizmoFunc) // and resize framebuffer
	{
		IFramebuffer& fb = *framebuffer;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
		ImGui::SetNextWindowSize({(float)fb.width, (float)fb.height+ImGui::GetFrameHeight()}, (window_mode==WM_FIXED_SIZE)?ImGuiCond_Always:ImGuiCond_Once);

		ImGuiWindowFlags vpWinFlag = ImGuiWindowFlags_NoCollapse;
		if( window_mode==WM_FIXED_RATIO ) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
				Viewport& vp = *(Viewport*)(data->UserData);

				float frameHeight = ImGui::GetFrameHeight();
				float aspectedWidth = vp.getAspect() * (data->DesiredSize.y - frameHeight);
				float aspectedHeight = data->DesiredSize.x / vp.getAspect() + frameHeight;

				if( data->DesiredSize.x <= aspectedWidth )
					data->DesiredSize.x = aspectedWidth;
				else
					data->DesiredSize.y = aspectedHeight;
			}, (void*)this);
			vpWinFlag |= ImGuiWindowFlags_NoDocking;
		}
		else if( window_mode==WM_FIXED_SIZE ) {
			vpWinFlag |= ImGuiWindowFlags_NoResize;
		}
		ImGui::Begin(name.c_str(), &is_opened, vpWinFlag);
		
		// for ignore draging
		bool isHoveredOnTitle = ImGui::IsItemHovered(); // when move window
		bool isWindowActivated = ImGui::IsItemActive(); // when resize window

		// update size
		auto contentSize = ImGui::GetContentRegionAvail();
		if( fb.width!=contentSize.x || fb.height !=contentSize.y ) {
			if(contentSize.x>10&&contentSize.y>10) // This is sometimes negative
				resize(contentSize.x, contentSize.y);
		}

		ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
		ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
		GLuint texID = framebuffer->getRenderedTex();
		if( texID!=0 ) {
			ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)fb.width, (float)fb.height}, ImVec2{0, 1}, ImVec2{1, 0});
		}
	
		is_focused = ImGui::IsWindowFocused();
		is_hovered = ImGui::IsItemHovered();
		is_dragged = isWindowActivated && !isHoveredOnTitle && ImGui::IsMouseDown(0);
		is_dragged |= (is_hovered||is_focused)&&(ImGui::IsMouseDown(1)||ImGui::IsMouseDown(2));

		prev_mouse_pos = mouse_pos;
		ImVec2 imMousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImVec2(0, ImGui::GetFrameHeight());
		mouse_pos = {imMousePos.x, imMousePos.y};
		mouse_off = mouse_pos - prev_mouse_pos;
		prev_mouse_pos = mouse_pos;

		ImGuiIO io = ImGui::GetIO();
		is_scrolled =  is_hovered&&(io.MouseWheel||io.MouseWheelH);
		scroll_off = {io.MouseWheelH, io.MouseWheel};
		
		if( guizmoFunc ) {
			guizmoFunc(*this);
			is_dragged &= !ImGuizmo::IsUsingAny();
		}
		if( is_dragged ) ImGui::SetMouseCursor(7);

		ImGui::End();
		ImGui::PopStyleVar();

		for( auto& cb : update_callbacks ) {
			cb(ImGui::GetIO().DeltaTime);
		}
		return is_opened;
	}
	void Viewport::resize(GLuint _width, GLuint _height)
	{
		framebuffer->resize(_width, _height);
		for( auto& cb : resize_callbacks ) {
			cb(_width, _height);
		}
	}
	const IFramebuffer& Viewport::getFb()
	{
		return *framebuffer;
	}
	void Viewport::setClearColor(glm::vec4 color)
	{
		framebuffer->clear_color = color;
	}
	const GLuint& Viewport::getWidth() const
	{
		return framebuffer->width;
	}
	const GLuint& Viewport::getHeight() const
	{
		return framebuffer->height;
	}
	const float& Viewport::getAspect() const
	{
		return framebuffer->aspect;
	}
}