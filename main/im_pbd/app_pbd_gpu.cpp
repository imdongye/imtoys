#include "app_pbd_gpu.h"
#include <limbrary/limgui.h>

using namespace lim;
using namespace glm;


AppPbdGPU::AppPbdGPU() : AppBase(1200, 780, APP_NAME, false)
	, viewport("viewport##pbd", new FramebufferMs())
{


	
}
AppPbdGPU::~AppPbdGPU()
{
}
void AppPbdGPU::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}
void AppPbdGPU::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();

	ImGui::Begin("skeletal ctrl");
	LimGui::PlotVal("dt", "ms", delta_time);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}
void AppPbdGPU::dndCallback(int cnt, const char **paths)
{
}