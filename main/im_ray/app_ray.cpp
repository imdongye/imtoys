#include "app_ray.h"
#include <limbrary/tools/limgui.h>

using namespace glm;
using namespace lim;

AppRay::AppRay(): AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferMs())
{
}
AppRay::~AppRay()
{
}
void AppRay::update()
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}
void AppRay::updateImGui()
{
	ImGui::DockSpaceOverViewport();
	
	// draw getFb() on viewport gui
	viewport.drawImGuiAndUpdateCam();

	ImGui::Begin("state##imray");
	ImGui::Text("asdf");
	ImGui::End();
}