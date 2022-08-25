#pragma once

#include "framebuffer.h"
namespace lim
{
class Viewport {
	glm::vec2 boundary_max, boundary_min;
	bool hovered, focused;
	GLuint id;
	static GLuint id_generator;
	lim::Framebuffer* framebuffer;
	lim::Camera* camera;
    char name[32];
public:
	GLuint width, height;
public:
	Viewport(lim::Framebuffer* fb, lim::Camera* cmr)
		: framebuffer(fb), camera(cmr), id(id_generator++)
		, boundary_max(0), boundary_min(0) {
        sprintf(name, "Viewport%d", id);
	}

	void renderImGui() {
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

        x = ImGui::GetCursorScreenPos().x;
        y = ImGui::GetCursorScreenPos().y;
        printf("%f %f", x, y);
        
		if( framebuffer->width != width
			|| framebuffer->height != height ) {
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
