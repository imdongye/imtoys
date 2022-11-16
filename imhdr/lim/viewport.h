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
		float aspect;
	private:
		const GLuint titlebar_height = 20;
		glm::ivec2 boundMin, boundMax, winPos;
	public:
		Viewport(Framebuffer* createdFB, GLuint _width=256, GLuint _height=256, bool fixedAspect=false)
			: id(id_generator++), name("Viewport"+std::to_string(id)), width(_width), height(_height)
			, fixed_aspect(fixedAspect), aspect(width/(float)(height+titlebar_height))
		{
			framebuffer = createdFB;
			framebuffer->resize(width, height);
		}
		virtual ~Viewport()
		{
			delete framebuffer;
		}
		void drawImGui()
		{
			ImGui::SetNextWindowSize({(float)width, (float)height+titlebar_height}, ImGuiCond_Once);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
				Viewport *vp = (Viewport*)(data->UserData);

				if( vp->fixed_aspect ) {

					float aspectedWidth = vp->aspect*data->DesiredSize.y;
					float aspectedHeight = data->DesiredSize.x/vp->aspect;

					if( data->DesiredSize.x <= aspectedWidth )
						data->DesiredSize.x = aspectedWidth;
					else
						data->DesiredSize.y = aspectedHeight;
				}
				vp->width = data->DesiredSize.x;
				vp->height = data->DesiredSize.y - vp->titlebar_height;
			}, (void*)this);

			ImGui::Begin(name.c_str());
			ImGui::SetScrollX(0);
			ImGui::SetScrollY(0);

			focused = ImGui::IsWindowFocused();
			hovered = ImGui::IsWindowHovered();
			dragging = focused && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

			winPos = imgui_modules::imToIvec(ImGui::GetWindowPos());
			boundMin = imgui_modules::imToIvec(ImGui::GetWindowContentRegionMin());
			boundMax = imgui_modules::imToIvec(ImGui::GetWindowContentRegionMax());
			mousePos = imgui_modules::imToIvec(ImGui::GetMousePos());
			mousePos = mousePos - winPos - boundMin;

			auto viewportPanelSize = ImGui::GetContentRegionAvail();
			width = viewportPanelSize.x;
			height = viewportPanelSize.y;

			if( dragging ) ImGui::SetMouseCursor(7);
			else framebuffer->resize(width, height);

			GLuint texID = framebuffer->getRenderedTex();
			if( texID!=0 )
				ImGui::Image(reinterpret_cast<void*>(texID), ImVec2{(float)width, (float)height}, ImVec2{0, 1}, ImVec2{1, 0});
			ImGui::End();
			ImGui::PopStyleVar();
		}
		bool isMouseHovered(int xPos, int yPos)
		{
			xPos-=winPos.x;
			yPos-=winPos.y;
			Logger::get()<<name;
			Logger::get().log("%d %d\n", xPos, yPos);
			Logger::get().log("%d %d\n", boundMin.x, boundMin.y);
			Logger::get().log("%d %d\n", boundMax.x, boundMax.y);
			bool ret= (xPos>boundMin.x)&&(xPos<boundMax.x)&&(yPos>boundMin.y)&&(yPos<boundMax.y);
			Logger::get().log("%d\n\n", ret);
			return ret;
		}
	};
} // ! namespace lim
#endif