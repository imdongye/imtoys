#include "app_pbd_cpu.h"
#include <limbrary/limgui.h>

using namespace lim;
using namespace glm;


AppPbdCPU::AppPbdCPU() : AppBaseCanvas3d(1200, 780, APP_NAME, false)
{


	
}
AppPbdCPU::~AppPbdCPU()
{
}
void AppPbdCPU::canvasUpdate()
{

}
void AppPbdCPU::canvasDraw() const
{
	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawSphere({0,1,0}, {1,0,0}, 1.f);

	drawQuad(vec3{0}, {0,1,0}, {0,0.3f,0});
}
void AppPbdCPU::canvasImGui()
{
	ImGui::Begin("pbd ctrl");
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}