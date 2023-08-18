
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


#include "viewport.h"
#include "glad/glad.h"
#include "app_pref.h"
#include "imgui_modules.h"

namespace lim {
	Viewport::Viewport(Framebuffer* createdFB, GLuint _width=256, GLuint _height=256, WindowMode wm=WM_FREE)
		: id(id_generator++), name("Viewport"+std::to_string(id)+"##vp"+AppPref::get().selectedAppName)
		, width(_width), height(_height), window_mode(wm), aspect(width/(float)height), mouse_pos(0), window_opened(true)
	{
		framebuffer = createdFB;
		framebuffer->resize(width, height);
		hovered = focused = dragging = false;
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
		dragging = focused && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

		static glm::ivec2 winPos;
		winPos = imgui_modules::imToIvec(ImGui::GetWindowPos());
		mouse_pos = imgui_modules::imToIvec(ImGui::GetMousePos());
		mouse_pos = mouse_pos - winPos - glm::ivec2(0, ImGui::GetFrameHeight());
		if( dragging ) ImGui::SetMouseCursor(7);

		// update size
		auto contentSize = ImGui::GetContentRegionAvail();
		if( width!=contentSize.x || height !=contentSize.y ) {
			width = contentSize.x;
			height = contentSize.y;
			aspect = contentSize.x/contentSize.y;
			framebuffer->resize(width, height);
			for( auto& [_,cb] : resize_callbacks ) {
				cb(width, height);
			}
		}

		GLuint texID = framebuffer->getRenderedTex();
		if( texID!=0 )
			ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)width, (float)height}, ImVec2{0, 1}, ImVec2{1, 0});

		ImGui::End();
		ImGui::PopStyleVar();
	}
}