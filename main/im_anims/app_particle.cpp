#include "app_particle.h"
#include <imgui.h>

using namespace lim;
using namespace glm;

AppParticle::AppParticle()
	: AppBase(1200, 780, APP_NAME), viewport("viewport##ptcl", new FramebufferNoDepth)
{
}
AppParticle::~AppParticle()
{
}
void AppParticle::update()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.f, 0.1f, 0.12f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

}
void AppParticle::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGui();

	ImGui::Begin("controller##ptcl");
	
	ImGui::End();
}