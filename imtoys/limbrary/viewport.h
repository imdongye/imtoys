//
//	2022-08-24 / im dong ye
//
//	TODO list:
//	1. initial framebuffer size setting
//	2. drag imgui demo 참고해서 다시짜기
//

#ifndef VIEWPORT_H
#define VIEWPORT_H

//#include "limclude.h"

namespace lim
{
	class Viewport
	{
	public:
		/* c++17 non-const static data member can initialize in declaration with inline keyword*/
		inline static GLuint id_generator = 0;
		GLuint id;
		const std::string name;
		Framebuffer* framebuffer;
		bool hovered, focused, dragging;
		GLuint width=0, height=0;
		glm::ivec2 mousePos;
		bool fixed_aspect;
		const float aspect;
	public:
		Viewport(Framebuffer* createdFB, GLuint _width=256, GLuint _height=256, bool fixedAspect=false)
			: id(id_generator++), name("Viewport"+std::to_string(id)), width(_width), height(_height)
			, fixed_aspect(fixedAspect), aspect(width/(float)height)
		{
			framebuffer = createdFB;
			framebuffer->setSize(width, height);
			hovered = focused = dragging = false;
		}
		virtual ~Viewport()
		{
			delete framebuffer;
		}
		void drawImGui()
		{
			static bool viewportOpen=true;
			ImGui::SetNextWindowSize({(float)width, (float)height+ImGui::GetFrameHeight()}, ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
				Viewport& vp = *(Viewport*)(data->UserData);

				if( vp.fixed_aspect ) {
					float frameHeight = ImGui::GetFrameHeight();
					float aspectedWidth = vp.aspect * (data->DesiredSize.y - frameHeight);
					float aspectedHeight = data->DesiredSize.x / vp.aspect + frameHeight;

					if( data->DesiredSize.x <= aspectedWidth )
						data->DesiredSize.x = aspectedWidth;
					else
						data->DesiredSize.y = aspectedHeight;
				}
			}, (void*)this);
			
			ImGui::Begin(name.c_str());//, &viewportOpen, ImGuiWindowFlags_DockNodeHost);

			focused = ImGui::IsWindowFocused();
			hovered = ImGui::IsWindowHovered();
			dragging = focused && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

			static glm::ivec2 winPos;
			winPos = imgui_modules::imToIvec(ImGui::GetWindowPos());
			mousePos = imgui_modules::imToIvec(ImGui::GetMousePos());
			mousePos = mousePos - winPos - glm::ivec2(0, ImGui::GetFrameHeight());
			if( dragging ) ImGui::SetMouseCursor(7);

			auto contentSize = ImGui::GetContentRegionAvail();
			width = contentSize.x;
			height = contentSize.y;
			framebuffer->setSize(width, height);

			GLuint texID = framebuffer->getRenderedTex();
			if( texID!=0 )
				ImGui::Image(reinterpret_cast<void*>(texID), ImVec2{(float)width, (float)height}, ImVec2{0, 1}, ImVec2{1, 0});

			ImGui::End();
			ImGui::PopStyleVar();
		}
	};
}
#endif