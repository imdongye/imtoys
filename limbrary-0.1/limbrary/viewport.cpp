#include <limbrary/viewport.h>
#include <imgui.h>

namespace lim {
	Viewport::Viewport(std::string_view _name, Framebuffer* createdFB)
		:name(_name)
	{
		framebuffer = createdFB;
		framebuffer->resize(width, height);
	}
	Viewport::~Viewport()
	{
		delete framebuffer;
	}
	void Viewport::drawImGui() // and resize framebuffer
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
		ImGui::SetNextWindowSize({(float)width, (float)height+ImGui::GetFrameHeight()}, (window_mode==WM_FIXED_SIZE)?ImGuiCond_Always:ImGuiCond_Once);

		ImGuiWindowFlags vpWinFlag = ImGuiWindowFlags_NoCollapse;
		if( window_mode==WM_FIXED_RATIO ) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
				Viewport& vp = *(Viewport*)(data->UserData);

				float frameHeight = ImGui::GetFrameHeight();
				float aspectedWidth = vp.aspect * (data->DesiredSize.y - frameHeight);
				float aspectedHeight = data->DesiredSize.x / vp.aspect + frameHeight;

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
		

		focused = ImGui::IsWindowFocused();
		hovered = ImGui::IsWindowHovered();
		dragging = !ImGui::IsItemHovered() && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

		ImVec2 imWinPos = ImGui::GetWindowPos();
		ImVec2 imMousePos = ImGui::GetMousePos();
		glm::ivec2 winPos ={imWinPos.x, imWinPos.y};
		glm::ivec2 mousePos = {imMousePos.x, imMousePos.y};
		mouse_pos = mousePos - winPos - glm::ivec2(0, ImGui::GetFrameHeight());
		if( dragging ) ImGui::SetMouseCursor(7);

		// update size
		auto contentSize = ImGui::GetContentRegionAvail();
		if( width!=contentSize.x || height !=contentSize.y ) {
			resize(contentSize.x, contentSize.y);
		}

		GLuint texID = framebuffer->getRenderedTex();
		if( texID!=0 )
			ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)width, (float)height}, ImVec2{0, 1}, ImVec2{1, 0});

		ImGui::End();
		ImGui::PopStyleVar();
	}
	void Viewport::resize(GLuint _width, GLuint _height)
	{
		width = _width;
		height = _height;
		aspect = width/(float)height;
		framebuffer->resize(width, height);
		for( auto& [_,cb] : resize_callbacks ) {
			cb(width, height);
		}
	}
}