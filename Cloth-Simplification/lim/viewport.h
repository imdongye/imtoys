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
		glm::vec2 boundary_max, boundary_min;
		bool hovered, focused;
		GLuint id;
		static GLuint id_generator;
		char name[32];
		lim::Camera* camera;
		lim::Framebuffer* framebuffer;
		GLuint width, height;
		float cursor_pos_x, cursor_pos_y;
	public:
		Viewport(lim::Camera* cmr)
			: boundary_max(0), boundary_min(0)
		{
			hovered = focused = false;
			id = id_generator++;
			sprintf_s(name, "Viewport%d", id);
			camera = cmr;
			framebuffer = new Framebuffer();
			width = height = 0;
		}

		void renderImGui()
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
			ImGui::Begin(name);
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

			ImGui::Begin("status");

			ImGui::Text("%.0f %.0f", x, y);
			ImGui::Text("%.0f %.0f", cursor_pos_x, cursor_pos_y);

			//ImGui::Text("%f %f", boundary_min.x, boundary_min.y);
			ImGui::Text(hovered?"hoverd":"not hovered");
			ImGui::Text(focused?"focused":"not focuesed");

			ImGui::End();
		}
	};
	GLuint Viewport::id_generator = 0;
} // ! namespace lim
#endif