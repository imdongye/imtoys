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
	bool draw_mesh = false;
}

static void resetApp() {
	pbd::SoftBody::Compliance tempComp;
	if(cur_body)
		tempComp = cur_body->compliance;
	simulator.clear();
	is_paused = true;
	draw_mesh = false;


	// bunny
	// {
	// 	Model md;
	// 	md.importFromFile("exports/simp_rst/bunny.obj");
	// 	md.setUnitScaleAndPivot({0,0,0}, 1.f);
	// 	Mesh& ms = *md.own_meshes[0];
	// 	tf = tf * md.tf_norm->mtx;
	// 	settings.update_buf = true;
	// }


	// cube
	// {
	// 	MeshCubeShared ms(0.5);
	// }


	// cloth
	{
		constexpr int nrWidth = 11;
		MeshPlane ms(0.7f, nrWidth, nrWidth, false, false);
		// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::PI90*0.1f)* glim::rotateY(glim::PI90*0.2f);
		mat4 tf = translate(vec3(0,2,0));
		for( vec3& p : ms.poss ) {
			p = vec3(tf*vec4(p,1));
		}
		cur_body = new pbd::SoftBody(ms, true, pbd::SoftBody::BendType::None);
	}


	cur_body->compliance = tempComp;
	cur_body->w_s[0] = 0.f;
	simulator.bodies.push_back( cur_body );
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
	if( !draw_mesh )
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
	if( !draw_mesh )
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
	for( const auto& c : body.c_stretchs ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {1,1,0} );
	}
}
void AppPbdCPU::canvasDraw() const
{
	for( auto b : simulator.bodies ) {
		drawBody( *b );
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
	if( ImGui::Checkbox("draw mesh", &draw_mesh) ) {
		cur_body->update_buf = draw_mesh;
	}

	ImGui::SliderFloat("a_distance", &cur_body->compliance.stretch, 0.f, 1.f, "%.6f");
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