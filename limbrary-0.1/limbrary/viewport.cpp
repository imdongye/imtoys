#include <limbrary/viewport.h>
#include <imgui.h>

namespace lim 
{
	Viewport::Viewport(std::string_view _name, Framebuffer* createdFB)
		:name(_name)
	{
		framebuffer = createdFB;
		framebuffer->resize(256, 256); // default size
	}
	Viewport::Viewport(Viewport&& src) noexcept
		: name(std::move(src.name))
		, resize_callbacks(std::move(src.resize_callbacks))
	{
		window_mode = src.window_mode;
		window_opened = src.window_opened;
		hovered = src.hovered;
		focused = src.focused;
		dragging = src.dragging;
		mouse_pos = src.mouse_pos;

		framebuffer= src.framebuffer;
		src.framebuffer = nullptr;
	}
	Viewport::~Viewport()
	{
		if( framebuffer )
			delete framebuffer;
	}
	bool Viewport::drawImGui() // and resize framebuffer
	{
		Framebuffer& fb = *framebuffer;
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
		ImGui::Begin(name.c_str(), &window_opened, vpWinFlag);
		
		static bool isHoveredOnTitle, isWindowActivated = false;
		isHoveredOnTitle = ImGui::IsItemHovered();
		isWindowActivated = ImGui::IsItemActive();

		// update size
		auto contentSize = ImGui::GetContentRegionAvail();
		if( fb.width!=contentSize.x || fb.height !=contentSize.y ) {
			resize(contentSize.x, contentSize.y);
		}

		ImVec2 imWinPos = ImGui::GetWindowPos();
		ImVec2 imMousePos = ImGui::GetMousePos();
		glm::ivec2 winPos ={imWinPos.x, imWinPos.y};
		glm::ivec2 mousePos = {imMousePos.x, imMousePos.y};
		mouse_pos = mousePos - winPos - glm::ivec2(0, ImGui::GetFrameHeight());

		GLuint texID = framebuffer->getRenderedTex();
		if( texID!=0 ) {
			ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)fb.width, (float)fb.height}, ImVec2{0, 1}, ImVec2{1, 0});
		}

		focused = ImGui::IsWindowFocused();
		hovered = ImGui::IsItemHovered();
		dragging = isWindowActivated && !isHoveredOnTitle && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));
		if( dragging ) ImGui::SetMouseCursor(7);

		ImGui::End();
		ImGui::PopStyleVar();
		
		return window_opened;
	}
	void Viewport::resize(GLuint _width, GLuint _height)
	{
		framebuffer->resize(_width, _height);
		for( auto& cb : resize_callbacks ) {
			cb(_width, _height);
		}
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