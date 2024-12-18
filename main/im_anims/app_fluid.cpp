#include "app_fluid.h"
#include "canvas.h"
#include <stb_image.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/asset_lib.h>

using namespace glm;

namespace
{
	lim::CanvasColor* canvas;
}

namespace lim
{
	AppFluid::AppFluid(): AppBase(1200, 780, APP_NAME), viewport(new FramebufferNoDepth)
	{
		canvas = new CanvasColor(viewport.fb_size);
		points.push_back({0,0});
	}
	AppFluid::~AppFluid()
	{
		delete canvas;
	}
	void AppFluid::update()
	{
		points[0].x = viewport.mouse_pos.x;
		points[0].y = viewport.content_size.y-viewport.mouse_pos.y;

		const float radius = 50.f;
		const float threshold = 0.5f;

		canvas->clear();

		for( int x=0; x<viewport.fb_size.x; x++ ) for( int y=0; y<viewport.fb_size.y; y++ ) {
			glm::vec2 pix = {x, y};
			float sum = 0.f;

			for( auto& pt : points ) {
				glm::vec2 diff = pt-pix;
				float pow_diff = glm::dot(diff, diff);
				float pow_radius = radius*radius;

				if( pow_diff < pow_radius ) {
					float pd_per_pr = pow_diff/pow_radius;
					float inv_pow_pd_per_pr = 1.f-pd_per_pr*pd_per_pr;
					float pow_inv = inv_pow_pd_per_pr*inv_pow_pd_per_pr;
					sum += pow_inv;
				}				
			}

			if( sum>threshold ) {
				canvas->at(x, y) = glm::vec3(1.);
			}
		}

		canvas->update();

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
		viewport.getFb().bind();
		
		drawTexToQuad(canvas->tex_id);
		viewport.getFb().unbind();


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.f, 0.1f, 0.12f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

	}
	void AppFluid::updateImGui()
	{
		//ImGui::DockSpaceOverViewport();

		// if( ImGui::BeginMainMenuBar() ) {
		// 	if( ImGui::BeginMenu("File") ) {
		// 		ImGui::EndMenu();
		// 	}
		// 	if( ImGui::BeginMenu("Edit") ) {
		// 		if( ImGui::MenuItem("Undo", "CTRL+Z") ) {}
		// 		if( ImGui::MenuItem("Redo", "CTRL+Y", false, false) ) {}  // Disabled item
		// 		ImGui::Separator();
		// 		if( ImGui::MenuItem("Cut", "CTRL+X") ) {}
		// 		if( ImGui::MenuItem("Copy", "CTRL+C") ) {}
		// 		if( ImGui::MenuItem("Paste", "CTRL+V") ) {}
		// 		ImGui::EndMenu();
		// 	}
		// 	ImGui::EndMainMenuBar();
		// }

		{
			static bool pOpen = true;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
				| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing 
				| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking;

			const float PAD = 10.0f;
			const ImGuiViewport* igvp = ImGui::GetMainViewport();
			ImVec2 work_pos = igvp->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
			ImVec2 work_size = igvp->WorkSize;
			ImVec2 window_pos, window_pos_pivot;
			window_pos.x = work_pos.x + PAD;
			window_pos.y = work_pos.y + PAD;
			window_pos_pivot.x = 0.0f;
			window_pos_pivot.y = 0.0f;
			ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

			ImGui::SetNextWindowBgAlpha(0.35f);
			ImGui::Begin("Example: Simple overlay", &pOpen, window_flags);
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("mouse pos : %4d, %4d", (int)points[0].x, (int)points[0].x);
			ImGui::End();
		}
		viewport.drawImGui();

		if( viewport.is_size_changed ) {
			canvas->resize(viewport.fb_size); 
		}

		if( ImGui::IsMouseClicked(0) && viewport.is_hovered ) {
			points.push_back({viewport.mouse_pos.x, viewport.content_size.y-viewport.mouse_pos.y});
		}
	}
}