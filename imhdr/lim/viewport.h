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
	private:
		glm::ivec2 boundMin, boundMax, winPos;
	public:
		Viewport()
			: id(id_generator++), name("Viewport"+std::to_string(id))
		{
			framebuffer = new MsFramebuffer();
		}
		virtual ~Viewport()
		{
			delete framebuffer;
		}
		void drawImGui()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
			ImGui::Begin(name.c_str());

			focused = ImGui::IsWindowFocused();
			hovered = ImGui::IsWindowHovered();
			dragging = focused && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

			winPos = imgui_modules::imToIvec(ImGui::GetWindowPos());
			boundMin = imgui_modules::imToIvec(ImGui::GetWindowContentRegionMin());
			boundMax = imgui_modules::imToIvec(ImGui::GetWindowContentRegionMax());
			mousePos = imgui_modules::imToIvec(ImGui::GetMousePos());
			mousePos = mousePos - winPos - boundMin;
			if( dragging ) ImGui::SetMouseCursor(7);

			auto viewportPanelSize = ImGui::GetContentRegionAvail();
			width = viewportPanelSize.x;
			height = viewportPanelSize.y;

			if( framebuffer->width != width
			   || framebuffer->height != height ) {
				framebuffer->resize(width, height);
			}

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