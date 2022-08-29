//
//	2022-08-24 / im dong ye
//
//	TODO list:
//	1. initial framebuffer size setting
//
#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "framebuffer.h"
namespace lim
{
	class Viewport
	{
	public:
		static GLuint id_generator;
		GLuint id;
		Camera* camera;
		Framebuffer* framebuffer;
		bool hovered, focused;
		glm::vec2 boundary_max, boundary_min;
		GLuint width, height;
		float cursor_pos_x, cursor_pos_y;
	public:
		Viewport(Camera* cmr)
			: boundary_max(0), boundary_min(0)
		{
			id = id_generator++;
			camera = cmr;
			framebuffer = new Framebuffer();
			hovered = focused = false;
			width = height = 0;
			cursor_pos_x = cursor_pos_x = 0;
		}
		virtual ~Viewport()
		{
			delete framebuffer;
		}
		void renderImGui()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

			ImGui::Begin(("Viewport"+std::to_string(id)).c_str());
			auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
			auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
			auto viewportOffset = ImGui::GetWindowPos();
			boundary_min ={viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
			boundary_max ={viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

			focused = ImGui::IsWindowFocused();
			hovered = ImGui::IsWindowHovered();

			auto viewportPanelSize = ImGui::GetContentRegionAvail();
			width = viewportPanelSize.x;
			height = viewportPanelSize.y;

			float x, y;

			x = ImGui::GetMousePos().x;
			y = ImGui::GetMousePos().y;
			//printf("%f %f\n", x, y);

			if( framebuffer->width != width
			   || framebuffer->height != height )
			{
				framebuffer->resize(width, height);
				camera->aspect = width/(float)height;
				camera->updateProjMat();
			}

			GLuint texID = framebuffer->colorTex;
			if( texID!=0 )
				ImGui::Image(reinterpret_cast<void*>(texID), ImVec2{(float)width, (float)height}, ImVec2{0, 1}, ImVec2{1, 0});
			ImGui::End();
			ImGui::PopStyleVar();
		}
	};
	GLuint Viewport::id_generator = 0;
} // ! namespace lim
#endif