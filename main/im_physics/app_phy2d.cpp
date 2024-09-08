/*
	unit : cm
*/

#include "app_phy2d.h"
#include <limbrary/program.h>
#include <vector>
#include <limbrary/tools/limgui.h>

using namespace std;
using namespace lim;
using namespace glm;


namespace {

	constexpr float grid_step = 1.f;
	bool enable_grid = true;
	vec2 view_point = vec2(0.f);
	float zoom_scale = 1.f;
	float world_width = 30.f;

	float world_height, scn_height;
	float aspect;
	float world_to_scn, scn_to_world;
	vec2 scn_origin;
}

static ImVec2 worldToScnPos(const vec2& p) {
	vec2 wLocalPos = p - view_point;
	vec2 sLocalPos = wLocalPos*world_to_scn;
	sLocalPos.y = scn_height - sLocalPos.y;
	return toIg(scn_origin + sLocalPos);
}



AppPhy2d::AppPhy2d()
	: AppBase(1200, 480, APP_NAME, true)
{
}
AppPhy2d::~AppPhy2d()
{
}
void AppPhy2d::update()
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void AppPhy2d::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	ImGui::Begin("controller##phy2d");
	{
		ImGui::Checkbox("Enable grid", &enable_grid);
		if( ImGui::Button("Reset") ) {
		}
		ImGui::TextWrapped("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");
		if( ImGui::CollapsingHeader("profiler") ) {
			LimGui::PlotVal("dt", "ms", delta_time);
			LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
		}
		ImGui::Text("%f %f", view_point.x, view_point.y);
	}
	ImGui::End();


	ImGui::SetNextWindowSize({ 300, 300 });
	ImGui::Begin("phy2d canvas");
	{
		const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
		const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
		const ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
		const vec2 screen_size = toGlm(canvas_sz);
		world_to_scn = zoom_scale * screen_size.x / world_width;
		scn_to_world = 1.f/world_to_scn;
		aspect = canvas_sz.y / canvas_sz.x;
		world_height = world_width * aspect;
		scn_height = canvas_sz.y;
		scn_origin = toGlm(canvas_p0);

		// draw border and background color
		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
		draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

		// this will catch our interactions
		ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
		const bool is_hovered = ImGui::IsItemHovered();
		const bool is_active = ImGui::IsItemActive();
		const bool is_scrolled =  is_hovered&&(io.MouseWheel||io.MouseWheelH);
		const vec2 scroll_off = {io.MouseWheelH, io.MouseWheel};


		if( is_active ) {
			if( ImGui::IsMouseDragging(ImGuiMouseButton_Right) ) {
				view_point += scn_to_world*vec2{-io.MouseDelta.x, io.MouseDelta.y};
			}
		}
		if( is_hovered ) {
			if( is_scrolled ) {
				zoom_scale *= 1.f + scroll_off.y*0.01f;
			}
		}

		draw_list->PushClipRect(canvas_p0, canvas_p1, true);
		if( enable_grid ) {
			for( float x = fmodf(-view_point.x, grid_step); x<world_width; x+=grid_step ) {
				float scnX = canvas_p0.x + world_to_scn * x;
				draw_list->AddLine({scnX, canvas_p0.y}, {scnX, canvas_p1.y}, IM_COL32(200, 200, 200, 40));
			}
			for( float y = fmodf(view_point.y, grid_step); y<world_height; y+=grid_step ) {
				float scnY = canvas_p0.y + world_to_scn * y;
				draw_list->AddLine({canvas_p0.x, scnY}, {canvas_p1.x, scnY}, IM_COL32(200, 200, 200, 40));
			}
		}
		draw_list->AddCircleFilled(worldToScnPos(vec2{0,0}), 1.f*world_to_scn, IM_COL32(200, 200, 200, 40));
		draw_list->PopClipRect();
	}
	ImGui::End();
}