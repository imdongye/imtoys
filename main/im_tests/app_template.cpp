#include "app_template.h"
#include <limbrary/tools/log.h>
#include <imgui.h>

using namespace lim;

AppTemplate::AppTemplate() : AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferNoDepth, "Viewport")
{
	key_callbacks[this] = [](int key, int scancode, int action, int mods) {
		// log::pure("%d\n", key);
		// log::pure("%d\n", key);
		// log::warn("%d\n", key);
		// log::err("%d\n", key);
	};
}
AppTemplate::~AppTemplate()
{
}
void AppTemplate::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewport.getFb().bind();
	viewport.getFb().unbind();
}
void AppTemplate::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGui();


	ImGui::Begin("test window2##template");
	ImGui::End();

	ImGui::Begin("test window##template");
	ImGui::Text("win size : %d %d", win_width, win_height);
	ImGui::Text("fb size  : %d %d", fb_width, fb_height);
	ImGui::Separator();


	ImGui::SliderInt("window mode", (int*)&viewport.window_mode, Viewport::WM_FREE, Viewport::WM_FIXED_SIZE);
	ImGui::SliderFloat("fixed_aspect", &viewport.fixed_aspect, 0.5f, 2.0f);
	ImGui::Text("name %s", viewport.name.c_str());
	ImGui::Text("is_open %d", viewport.is_opened);
	ImGui::Text("is_hovered %d", viewport.is_hovered);
	ImGui::Text("is_focused %d", viewport.is_focused);
	ImGui::Text("is_dragged %d", viewport.is_dragged);
	ImGui::Text("is_hidden %d", viewport.is_hidden);
	ImGui::Text("is_size_changed %d", viewport.is_size_changed);

	ImGui::Text("fb_size %d %d", viewport.fb_size.x, viewport.fb_size.y);

	ImGui::Text("content_pos %f %f", viewport.content_pos.x, viewport.content_pos.y);
	ImGui::Text("content_size %f %f", viewport.content_size.x, viewport.content_size.y);

	ImGui::Text("mouse_pos %f %f", viewport.mouse_pos.x, viewport.mouse_pos.y);;
	ImGui::Text("mouse_uv_pos%f %f", viewport.mouse_uv_pos.x, viewport.mouse_uv_pos.y);

	ImGui::End();
}