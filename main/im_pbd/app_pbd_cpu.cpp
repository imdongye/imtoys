#include "app_pbd_cpu.h"
#include <limbrary/limgui.h>
#include "pbd/pbd.h"
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/glm_tools.h>
using namespace lim;
using namespace glm;


namespace {
	AppPbdCPU* g_app = nullptr;
	pbd::Simulator simulator;


	bool is_paused = true;
}

static void resetApp() {
	simulator.bodies.clear();

	mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(H_PI*0.1f);
	MeshCubeShared ms(0.5);
	// MeshPlane ms(1.f, ......5, 5, false, false);
	for( vec3& p : ms.poss ) {
		p = vec3(tf*vec4(p,1));
	}
	pbd::SoftBody::Settings settings;
	simulator.bodies.emplace_back( ms, settings );
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
	for( const auto& c : body.c_distances ) {
		g_app->drawCylinder( vec3{body.xw_s[c.idx_ps.x]}, vec3{body.xw_s[c.idx_ps.y]}, {1,0,0} );
	}
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
	if( is_paused && ImGui::Button("one step 0.5sec") ) {
		float elapsed = 0.f;
		while( elapsed<0.1f ) {
			simulator.update(delta_time);
			elapsed += delta_time;
		}
	}
	if ( ImGui::Button("reset") ) {
		resetApp();
	}
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}