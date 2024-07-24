#include "app_pbd_cpu.h"
#include <limbrary/limgui.h>
#include "pbd/pbd.h"
#include <limbrary/model_view/mesh_maked.h>
using namespace lim;
using namespace glm;


namespace {
	AppPbdCPU* g_app = nullptr;
	pbd::Simulator simulator;


	bool is_paused = true;
}

static void resetApp() {
	mat4 tf = translate(vec3(0,2,0));
	simulator.bodies.clear();
	simulator.bodies.emplace_back( MeshCubeShared(0.5f) );
	// simulator.meshes.emplace_back( MeshPlane(1.f, 5, 5, false, false), tf );
}
static void deleteApp() {
	simulator.bodies.clear();
}

AppPbdCPU::AppPbdCPU() : AppBaseCanvas3d(1200, 780, APP_NAME, false)
{
	g_app = this;
	resetApp();
}
AppPbdCPU::~AppPbdCPU()
{
	deleteApp();
}
void AppPbdCPU::canvasUpdate()
{
	if( is_paused )
		return;
	simulator.update(delta_time);
}
static void drawBody(const pbd::SoftBody& body) {
	for(const vec4& p : body.xw_s) {
		g_app->drawSphere(vec3{p}, {1,0,0});
	}
	// for( const pbd::Edge& e : mesh.edges ) {
	// 	g_app->drawCylinder( vec3{mesh.xw_s[e.idx_ps.x]}, vec3{mesh.xw_s[e.idx_ps.y]}, {1,0,0} );
	// }
}
void AppPbdCPU::canvasDraw() const
{
	for( const pbd::SoftBody& body : simulator.bodies ) {
		drawBody( body );
	}

	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawQuad(vec3{0}, {0,1,0}, {0,0.3f,0});
}
void AppPbdCPU::canvasImGui()
{
	ImGui::Begin("pbd ctrl");
	ImGui::Checkbox("pause", &is_paused);
	if( is_paused && ImGui::Button("one step") ) {
		for(int i=0; i<100; i++)
			simulator.update(delta_time);
	}
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}