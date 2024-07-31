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
	AppPbdCpu* g_app = nullptr;
	pbd::Simulator simulator;
	pbd::SoftBody ptcl_test;
	pbd::SoftBody* cur_body = nullptr;
	pbd::ColliderPlane c_ground;
	pbd::ColliderSphere c_sphere({0,0.5f,0}, 0.3f);
	bool is_paused = true;
	bool draw_mesh = false;
	int nr_cloth_width = 1;
}



static void resetApp() {
	pbd::SoftBody::Compliance tempComp;
	if(cur_body) {
		tempComp = cur_body->compliance;
		delete cur_body;
	}
	simulator.clearRefs();
	simulator.static_colliders.push_back(&c_ground);
	simulator.static_colliders.push_back(&c_sphere);
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


	// cloth0
	{
		int nrShear = 1;
		float mass = 1.f;
		auto bendType = pbd::SoftBody::BendType::Dihedral;
		MeshPlane ms(1.f, nr_cloth_width, nr_cloth_width, false, false);
		mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
		// mat4 tf = translate(vec3(0,2,0));
		for( vec3& p : ms.poss ) {
			p = vec3(tf*vec4(p,1));
		}
		cur_body = new pbd::SoftBody(ms, nrShear, bendType, mass);
		cur_body->w_s[0] = 0.f;
		cur_body->w_s[nr_cloth_width] = 0.f;
	}

	// cloth
	// {
	// 	int nrShear = 2;
	// 	auto bendType = pbd::SoftBody::BendType::Distance;
	// 	MeshCloth ms(vec2(1.f, 1.f), 0.3f);
	// 	mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	// mat4 tf = translate(vec3(0,2,0));
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nrShear, bendType);
	// 	cur_body->w_s[0] = 0.f;
	// }

	// cloth cell
	// {
	// 	int nrShear = 0;
	// 	float width = 1.f;
	// 	int nrWidth = 1;
	// 	auto bendType = pbd::SoftBody::BendType::None;
	// 	MeshCloth ms(vec2(1.f), width/nrWidth);
	// 	mat4 tf = translate(vec3(0,2,0));
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nrShear, bendType);
	// 	cur_body->w_s[0] = 0.f;
	// 	cur_body->w_s[nrWidth] = 0.f;
	// }

	cur_body->compliance = tempComp;
	simulator.bodies.push_back( cur_body );
	simulator.bodies.push_back( &ptcl_test );
}
static void deleteApp() {
	simulator.bodies.clear();
}

AppPbdCpu::AppPbdCpu() : AppBaseCanvas3d(1200, 780, APP_NAME, false, 10, 1000000, 10000000)
{
	max_fps = 60;
	prog_ms.attatch("mvp.vs").attatch("ndl.fs").link();
	g_app = this;
	resetApp();
}
AppPbdCpu::~AppPbdCpu()
{
	deleteApp();
}
void AppPbdCpu::canvasUpdate()
{
	if( is_paused )
		return;
	simulator.update(delta_time);
}


void AppPbdCpu::customDrawShadow(const mat4& mtx_View, const mat4& mtx_Proj) const
{
	if( !draw_mesh )
		return;
	Program& sProg = AssetLib::get().prog_shadow_static;
	sProg.use();
	sProg.setUniform("mtx_View", mtx_View);
	sProg.setUniform("mtx_Proj", mtx_Proj);
	sProg.setUniform("mtx_Model", glm::mat4(1));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void AppPbdCpu::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	if( !draw_mesh )
		return;
	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}



static void drawBody(const pbd::SoftBody& body) {
	for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawSphere(body.poss[i], (body.w_s[i]>0)?vec3{1,1,0}:vec3{1,0,0});
	}
	for( const auto& c : body.c_stretchs ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {1,1,0} );
		// for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.poss[c.idx_ps[i]], body.poss[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_shears ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {0,1,1} );
		// for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.poss[c.idx_ps[i]], body.poss[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_dist_bends ) {
		g_app->drawCylinder( body.poss[c.idx_ps.x], body.poss[c.idx_ps.y], {0.7,0.1,0} );
		// for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.poss[c.idx_ps[i]], body.poss[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_dih_bends ) {
		g_app->drawCylinder( body.poss[c.idx_ps.z], body.poss[c.idx_ps.w], {0,0,1} );
		for( int i=0; i<4; i++ ) {
			g_app->drawCylinder( body.poss[c.idx_ps[i]], body.poss[c.idx_ps[i]]+c.dPi[i], {0,0,1} );
		}
	}
}
void AppPbdCpu::canvasDraw() const
{
	if( !draw_mesh ) {
		for( auto b : simulator.bodies ) {
			drawBody( *b );
		}
	}

	drawSphere(c_sphere.c, {1,0,0}, c_sphere.r*2.f);

	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawQuad(vec3{0}, c_ground.n, {0,0.3f,0});
}



namespace {
	struct PickingInfo {
		bool picked = false;
		bool beforeFixed = false;
		pbd::SoftBody* body;
		int ptcl_idx;
		vec3& pos(){
			return body->poss[ptcl_idx];
		}
		vec3& vel() {
			return body->v_s[ptcl_idx];
		}
		float& w() {
			return body->w_s[ptcl_idx];
		}
	};
	PickingInfo picked_info;
}

static void updatePicking( const vec3& rayDir, const vec3& rayOri ) {
	float minDepth = FLT_MAX;

	for(pbd::SoftBody* body : simulator.bodies) {
		for(int i=0; i<body->nr_ptcls; i++) {
			vec3 toObj = body->poss[i] - rayOri;
			float distFromLine = glm::length( glm::cross(rayDir, toObj) );
			if( distFromLine < 0.04f ) {
				float distProjLine = glm::dot(rayDir, toObj);
				if( distProjLine>0 && minDepth>distProjLine ) {
					minDepth = distProjLine;
					picked_info.picked = true;
					picked_info.body = body;
					picked_info.ptcl_idx = i;
					picked_info.beforeFixed = (body->w_s[i] == 0.f);
				}
			}
		}
	}
}
void AppPbdCpu::canvasImGui()
{
	ImGui::Begin("pbd ctrl");
	ImGui::Checkbox("pause", &is_paused);
	if( is_paused ) {
		if( ImGui::Button("next 0.05sec") ) {
			float elapsed = 0.f;
			while( elapsed<0.05f ) {
				simulator.update(delta_time);
				elapsed += delta_time;
			}
		}
		if( ImGui::Button("one step") ) {
			int tempIter = simulator.nr_steps;
			simulator.nr_steps = 1;
			simulator.update(delta_time);
			simulator.nr_steps = tempIter;
		}
		if( ImGui::SliderInt("cloth width", &nr_cloth_width, 1, 20) ) {
			resetApp();
		}
	}
	if( ImGui::Button("reset") ) {
		resetApp();
	}

	if( ImGui::Checkbox("draw mesh", &draw_mesh) ) {
		cur_body->upload_to_buf = draw_mesh;
	}
	if( ImGui::CollapsingHeader("compliance") ) {
		ImGui::Text("#stretch %d", cur_body->c_stretchs.size());
		ImGui::Text("#shear %d", cur_body->c_shears.size());
		ImGui::Text("#dist_bend %d", cur_body->c_dist_bends.size());
		ImGui::Text("#dih_bend %d", cur_body->c_dih_bends.size());
		ImGui::Text("#iso_bend %d", cur_body->c_iso_bends.size());
		ImGui::SliderFloat("stretch", &cur_body->compliance.stretch, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("shear", &cur_body->compliance.shear, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("dist_bend", &cur_body->compliance.dist_bend, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("dih_bend", &cur_body->compliance.dih_bend, 0.f, 1.f, "%.6f");

		ImGui::SliderFloat("air_drag", &simulator.air_drag, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground friction", &c_ground.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground restitution", &c_ground.restitution, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere friction", &c_sphere.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere restitution", &c_sphere.restitution, 0.f, 1.f, "%.6f");

	}
	
	LimGui::PlotVal("dt", "ms", delta_time*1000.f);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::SliderInt("# steps", &simulator.nr_steps, 1, 50);
	ImGui::SliderInt("max fps", &max_fps, -1, 300);
	ImGui::End();

	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		updatePicking(mouseRay, vp.camera.pos);
		if( picked_info.picked ) {
			picked_info.w() = 0.f;
			picked_info.vel() = vec3(0.f);
		}
	}
	if(picked_info.picked && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		const vec3 toObj = picked_info.pos() - vp.camera.pos;
		const vec3 mouseRay = vp.getMousePosRayDir();
		const float depth = dot(vp.camera.front, toObj)/dot(vp.camera.front, mouseRay);
		picked_info.pos() = depth*mouseRay+vp.camera.pos;
	}
	if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		picked_info.picked = false;
	}
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		updatePicking(mouseRay, vp.camera.pos);
		if( picked_info.picked ) {
			picked_info.w() = picked_info.body->inv_ptcl_mass;
		}
	}
	if(ImGui::IsKeyPressed(ImGuiKey_B, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		ptcl_test.addPtcl(vp.camera.pos, 1/0.1f, 10.f*mouseRay);
	}
	if(ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
		is_paused = !is_paused;
	}
}