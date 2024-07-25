#include "app_pbd_cpu.h"
#include <limbrary/limgui.h>
#include "pbd/pbd.h"
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/model.h>
#include <limbrary/glm_tools.h>
#include <limbrary/asset_lib.h>
using namespace lim;
using namespace glm;


namespace {
	AppPbdCPU* g_app = nullptr;
	pbd::Simulator simulator;

	pbd::SoftBody::Settings settings;
	pbd::SoftBody* body;
	bool is_paused = true;
}

static void resetApp() {
	simulator.clear();
	mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(H_PI*0.1f)* glim::rotateY(H_PI*0.2f);

	// Model md;
	// md.importFromFile("exports/simp_rst/bunny.obj");
	// md.setUnitScaleAndPivot({0,0,0}, 1.f);
	// Mesh& ms = *md.own_meshes[0];
	// tf = tf * md.tf_norm->mtx;
	// settings.update_buf = true;

	MeshCubeShared ms(0.5);
	// MeshPlane ms(1.f, 5, 5, false, false);

	for( vec3& p : ms.poss ) {
		p = vec3(tf*vec4(p,1));
	}
	simulator.bodies.push_back( new pbd::SoftBody(ms, settings) );
	body = simulator.bodies.back();
	// simulator.bodies.back()->w_s[0] = 0.f;
	// simulator.bodies.back()->w_s[1] = 0.f;
}
static void deleteApp() {
	simulator.bodies.clear();
}

AppPbdCPU::AppPbdCPU() : AppBaseCanvas3d(1200, 780, APP_NAME, false, 10, 1000000, 10000000)
{
	prog_ms.attatch("mvp.vs").attatch("ndl.fs").link();
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


void AppPbdCPU::customDrawShadow(const mat4& mtx_View, const mat4& mtx_Proj) const
{
	// Program& prog = AssetLib::get().prog_shadow_static;
	// prog.use();
	// prog.setUniform("mtx_View", mtx_View);
	// prog.setUniform("mtx_Proj", mtx_Proj);
	// prog.setUniform("mtx_Model", glm::mat4(1));

	// for( auto body : simulator.bodies ) {
	// 	body->bindAndDrawGL();
	// }
}
void AppPbdCPU::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	// prog_ms.use();
	// cam.setUniformTo(prog_ms);
	// lit.setUniformTo(prog_ms);
	// prog_ms.setUniform("mtx_Model", glm::mat4(1));

	// for( auto body : simulator.bodies ) {
	// 	body->bindAndDrawGL();
	// }
}
static void drawBody(const pbd::SoftBody& body) {
	for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawSphere(body.poss[i], (body.w_s[i]>0)?vec3{1,1,0}:vec3{1,0,0});
	}
	for( const auto& c : body.c_distances ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {1,1,0} );
	}
}
void AppPbdCPU::canvasDraw() const
{
	for( auto body : simulator.bodies ) {
		drawBody( *body );
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
	// ImGui::SliderFloat("a_distance", &settings.a_distance, 0.f, 0.0001f, "%.6f");
	ImGui::SliderFloat("a_distance", &body->settings.a_distance, 0.f, 0.1f, "%.6f");
	if( ImGui::Button("reset") ) {
		resetApp();
	}
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::End();
}