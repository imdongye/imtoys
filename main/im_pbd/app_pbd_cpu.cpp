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

	pbd::SoftBody* cur_body;
	bool is_paused = true;
	bool canvas_view_mode = true;
}

static void resetApp() {
	pbd::SoftBody::Settings settings;
	if(cur_body)
		settings = cur_body->settings;
	simulator.clear();
	// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(PI90*0.1f)* glim::rotateY(PI90*0.2f);
	mat4 tf = translate(vec3(0,2,0));

	// Model md;
	// md.importFromFile("exports/simp_rst/bunny.obj");
	// md.setUnitScaleAndPivot({0,0,0}, 1.f);
	// Mesh& ms = *md.own_meshes[0];
	// tf = tf * md.tf_norm->mtx;
	// settings.update_buf = true;
	// settings.apply_volume_constraint = true;

	// MeshCubeShared ms(0.5);

	constexpr int nrWidth = 11;
	MeshPlane ms(0.7f, nrWidth, nrWidth, false, false);

	for( vec3& p : ms.poss ) {
		p = vec3(tf*vec4(p,1));
	}
	settings.bendType = pbd::SoftBody::Settings::BendType::Isometric;

	simulator.bodies.push_back( new pbd::SoftBody(ms, settings) );
	cur_body = simulator.bodies.back();
	simulator.bodies.back()->w_s[0] = 0.f;
	simulator.bodies.back()->w_s[nrWidth] = 0.f;
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
	if( canvas_view_mode )
		return;
	Program& prog = AssetLib::get().prog_shadow_static;
	prog.use();
	prog.setUniform("mtx_View", mtx_View);
	prog.setUniform("mtx_Proj", mtx_Proj);
	prog.setUniform("mtx_Model", glm::mat4(1));

	for( auto body : simulator.bodies ) {
		body->bindAndDrawGL();
	}
}
void AppPbdCPU::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	if( canvas_view_mode )
		return;
	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));

	for( auto body : simulator.bodies ) {
		body->bindAndDrawGL();
	}
}
static void drawBody(const pbd::SoftBody& body) {
	for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawSphere(body.poss[i], (body.w_s[i]>0)?vec3{1,1,0}:vec3{1,0,0});
	}
	for( const auto& c : body.c_distances ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {1,1,0} );
	}
	for( const auto& c : body.c_bendings ) {
		g_app->drawCylinder( body.poss[c.idx_ps.z], body.poss[c.idx_ps.w], {0,0,1} );
		for( int i=0; i<4; i++ ) {
			g_app->drawCylinder( body.poss[c.idx_ps[i]], body.poss[c.idx_ps[i]]+c.dCi[i], {0,0,1} );
		}
	}
}
void AppPbdCPU::canvasDraw() const
{
	if( canvas_view_mode ) {
		for( auto b : simulator.bodies ) {
			drawBody( *b );
		}
	}

	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawQuad(vec3{0}, {0,1,0}, {0,0.3f,0});
}
namespace {
	pbd::SoftBody* picked_body;
	int picked_ptcl_idx;
}

static void pickClosestPtclInRay( const vec3& rayDir, const vec3& rayOri ) {
	float minDepth = FLT_MAX;
	picked_body = nullptr;

	for(pbd::SoftBody* body : simulator.bodies) {
		for(int i=0; i<body->nr_ptcls; i++) {
			vec3 toObj = body->poss[i] - rayOri;
			float distFromLine = glm::length( glm::cross(rayDir, toObj) );
			if( distFromLine < 0.02f ) {
				float distProjLine = glm::dot(rayDir, toObj);
				if( distProjLine>0 && minDepth>distProjLine ) {
					minDepth = distProjLine;
					picked_body = body;
					picked_ptcl_idx = i;
				}
			}
		}
	}
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
	ImGui::Checkbox("canvas view", &canvas_view_mode);
	// ImGui::SliderFloat("a_distance", &settings.a_distance, 0.f, 0.0001f, "%.6f");
	ImGui::SliderFloat("a_distance", &cur_body->settings.a_distance, 0.f, 1.f, "%.6f");
	ImGui::SliderFloat("a_bending", &cur_body->settings.a_bending, 0.f, 10.f, "%.6f");
	ImGui::SliderFloat("a_volume", &cur_body->settings.a_volume, 0.f, 1.f, "%.6f");
	if( ImGui::Button("reset") ) {
		resetApp();
	}
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::SliderInt("max fps", &max_fps, -1, 300);
	ImGui::End();

	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickClosestPtclInRay(mouseRay, vp.camera.pos);
		if( picked_body ) {
			if ( picked_body->w_s[picked_ptcl_idx] > 0.f ) {
				picked_body->w_s[picked_ptcl_idx] = 0.f;
			} else {
				picked_body->w_s[picked_ptcl_idx] = 1.f;
			}
		}
	} else if(picked_body && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		vec3& dstP = picked_body->poss[picked_ptcl_idx];
		const vec3 toObj = dstP-vp.camera.pos;
		const vec3 mouseRay = vp.getMousePosRayDir();
		const float depth = dot(vp.camera.front, toObj)/dot(vp.camera.front, mouseRay);
		dstP = depth*mouseRay+vp.camera.pos;
	} else if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		picked_body = nullptr;
	}
}