//
//	2022-08-24 / im dong ye
//
//	TODO list:
//	1. initial framebuffer size setting
//	2. drag imgui demo 참고해서 다시짜기
//

#ifndef VIEWPORT_H
#define VIEWPORT_H

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
		GLuint width, height;
		glm::ivec2 mousePos;
	public:
		Viewport()
			: id(id_generator++), name("Viewport"+std::to_string(id))
		{
			framebuffer = new MsFramebuffer();
			hovered = focused = false;
			width = height = 0;
		}
		virtual ~Viewport()
		{
			delete framebuffer;
		}
		void drawImGui()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
			ImGui::Begin(name.c_str());
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();

			focused = ImGui::IsWindowFocused();
			hovered = ImGui::IsWindowHovered();
			dragging = focused && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2));

			if( dragging ) {
				glm::ivec2 winPos, vpPos;
				winPos = imgui_modules::imToIvec(ImGui::GetWindowPos());
				vpPos = imgui_modules::imToIvec(ImGui::GetWindowContentRegionMin());
				mousePos = imgui_modules::imToIvec(ImGui::GetMousePos());
				mousePos = mousePos - winPos - vpPos;
				ImGui::SetMouseCursor(7);
				//ImGui::Text("%f %f", mousePos.x, mousePos.y);
			}

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
	};
} // ! namespace lim
#endif