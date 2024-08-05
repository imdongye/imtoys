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
	pbd::PhyScene phy_scene;
	pbd::SoftBody ptcl_test;
	pbd::SoftBody* cur_body = nullptr;
	pbd::ColliderPlane c_ground;
	pbd::ColliderSphere c_sphere({0,0.5f,0}, 0.3f);
	bool is_paused = true;
	bool draw_mesh = false;
	bool draw_dpi_dir = false;
	int nr_cloth_width = 13;
	float cloth_width = 1.3f;
	int nr_shear = 1;
	float time_speed = 1.f;
	float body_mass = 1.f;
	bool fix_start = true;
	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_ISOMETRIC;
}



static void resetApp() {
	pbd::SoftBody::Compliance tempComp;
	if(cur_body) {
		tempComp = cur_body->compliance;
		delete cur_body;
	}
	is_paused = true;
	draw_mesh = false;


	// bunny
	// {
	// 	int nrShear = 1;
	// 	float mass = 1.f;
	// 	Model md;
	// 	md.importFromFile("exports/simp_rst/bunny.obj");
	// 	md.setUnitScaleAndPivot({0,-1,0}, 0.5f);
	// 	Mesh& ms = *md.own_meshes[0];
	// 	mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nrShear, bendType, mass);
	// 	cur_body->w_s[0] = 0.f;
	// 	// cur_body->w_s[nr_cloth_width] = 0.f;
	// 	// cur_body->upload_to_buf = true;
	// }


	// duck
	// {
	// 	int nrShear = 1;
	// 	float mass = 1.f;
	// 	Model md;
	// 	md.importFromFile("exports/simp_rst/Duck/Duck.obj");
	// 	md.setUnitScaleAndPivot({0,-1,0}, 0.5f);
	// 	Mesh& ms = *md.own_meshes[0];
	// 	mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nrShear, bendType, mass);
	// 	cur_body->w_s[0] = 0.f;
	// 	// cur_body->w_s[nr_cloth_width] = 0.f;
	// 	// cur_body->upload_to_buf = true;
	// }

	// cube
	// {
	// 	float mass = 1.f;
	// 	MeshCubeShared ms(0.5f);
	// 	// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	mat4 tf = translate(vec3(0,0,0));
	// 	// mat4 tf = translate(vec3(0,2,0));
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nr_shear, bend_type, mass);
	// 	cur_body->w_s[0] = 0.f;
	// }
	// {
	// 	float mass = 1.f;
	// 	MeshCube ms(0.5f);
	// 	// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	mat4 tf = translate(vec3(0,2,0));
	// 	// mat4 tf = translate(vec3(0,2,0));
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nr_shear, bend_type, mass);
	// 	cur_body->w_s[0] = 0.f;
	// }


	// sphere
	// {
	// 	float mass = 1.f;
	// 	bool enableVolume = true;
	// 	MeshSphereShared ms(0.5f, 4, 4, false);
	// 	// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	// mat4 tf = translate(vec3(0,0,0));
	// 	mat4 tf = translate(vec3(0,2,0));
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nr_shear, bend_type, mass, enableVolume);
	// 	cur_body->w_s[0] = 0.f;
	// }


	// cloth plane
	{
		// auto bendType = pbd::SoftBody::BendType::Dihedral;
		MeshPlane ms(cloth_width, nr_cloth_width, nr_cloth_width, false, false);
		// mat4 tf = translate(vec3(0,2,0));
		// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
		mat4 tf = translate(vec3(0,2,0)) * glim::rotateZ(-glim::pi45*0.05f);
		for( vec3& p : ms.poss ) {
			p = vec3(tf*vec4(p,1));
		}
		cur_body = new pbd::SoftBody(ms, nr_shear, bend_type, body_mass);
		if( fix_start ) {
			cur_body->w_s[0] = 0.f;
			cur_body->w_s[nr_cloth_width] = 0.f;
		}
	}

	// cloth
	// {
	// 	MeshCloth ms(vec2(cloth_width), cloth_width/nr_cloth_width);
	// 	// mat4 tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f);
	// 	// mat4 tf = translate(vec3(0,2,0));
	// 	mat4 tf = translate(vec3(0,2,0)) * glim::rotateZ(-glim::pi45*0.05f);
	// 	for( vec3& p : ms.poss ) {
	// 		p = vec3(tf*vec4(p,1));
	// 	}
	// 	cur_body = new pbd::SoftBody(ms, nr_shear, bend_type);
	// 	if( fix_start ) {
	// 		cur_body->w_s[0] = 0.f;
	// 		cur_body->w_s[nr_cloth_width] = 0.f;
	// 	}
	// }
	cur_body->compliance = tempComp;

	phy_scene.bodies.clear();
	phy_scene.bodies.push_back( cur_body );
	phy_scene.bodies.push_back( &ptcl_test );

	phy_scene.colliders.clear();
	phy_scene.colliders.push_back( &c_ground );
	phy_scene.colliders.push_back( &c_sphere );
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
	delete cur_body;
}
void AppPbdCpu::canvasUpdate()
{
	if( is_paused )
		return;
	phy_scene.update(delta_time*time_speed);
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

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void AppPbdCpu::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	if( !draw_mesh )
		return;
	glEnable(GL_CULL_FACE);

	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_CULL_FACE);

}



static void drawBody(const pbd::SoftBody& body) {
	for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawSphere(body.x_s[i], (body.w_s[i]>0)?vec3{1,1,0}:vec3{1,0,0});
	}
	for( const auto& c : body.c_stretchs ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], {1,1,0} );
		// if( draw_dpi_dir ) for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_shears ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], {0,1,1} );
		// for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_dist_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], {0.7,0.1,0}, 0.03f );
		if( draw_dpi_dir ) for( int i=0; i<2; i++ ) {
			g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i], {0,0,1} );
		}
	}
	for( const auto& c : body.c_dih_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], {0,0,1} );
		if( draw_dpi_dir ) for( int i=0; i<4; i++ ) {
			g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i], {0,0,1} );
		}
	}
	for( const auto& c : body.c_iso_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], {0,0,1} );
		if( draw_dpi_dir ) for( int i=0; i<4; i++ ) {
			g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i], {0,0,1} );
		}
	}
	// for( int i=0; i<body.nr_ptcls; i++ ) {
	// 	g_app->drawCylinder( body.x_s[i], body.x_s[i]+body.c_g_volume.dPi[i], {0,0,1} );
	// }
}
void AppPbdCpu::canvasDraw() const
{
	if( !draw_mesh ) {
		for( auto b : phy_scene.bodies ) {
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
			return body->x_s[ptcl_idx];
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

	for(pbd::SoftBody* body : phy_scene.bodies) {
		for(int i=0; i<body->nr_ptcls; i++) {
			vec3 toObj = body->x_s[i] - rayOri;
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
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Checkbox("pause", &is_paused);
		if( is_paused ) {
			if( ImGui::Button("next 0.05sec") ) {
				float elapsed = 0.f;
				while( elapsed<0.05f ) {
					phy_scene.update(delta_time);
					elapsed += delta_time;
				}
			}
			if( ImGui::Button("one sub step") ) {
				int tempIter = cur_body->nr_steps;
				cur_body->nr_steps = 1;
				phy_scene.update(delta_time);
				cur_body->nr_steps = tempIter;
			}
			if( ImGui::SliderFloat("cloth width", &cloth_width, 0.5f, 3.f) ) {
				resetApp();
			}
			if( ImGui::SliderInt("nr cloth slices", &nr_cloth_width, 1, 20) ) {
				resetApp();
			}
			if( ImGui::SliderInt("nr shears", &nr_shear, 0, 2) ) {
				resetApp();
			}
			if( ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4) ) {
				resetApp();
			}
			if( ImGui::Checkbox("fix", &fix_start) ) {
				resetApp();
			}
			if( ImGui::SliderFloat("body mass", &body_mass, 0.01f, 2.f) ) {
				resetApp();
			}
		}
		if( ImGui::Button("reset") ) {
			resetApp();
		}
	}
	
	if( ImGui::CollapsingHeader("draw&run") ) {
		ImGui::Checkbox("draw dPi dir draw", &draw_dpi_dir);

		if( ImGui::Checkbox("draw mesh", &draw_mesh) ) {
			cur_body->upload_to_buf = draw_mesh;
		}
		ImGui::SliderInt("# steps", &cur_body->nr_steps, 1, 50);
		ImGui::SliderInt("max fps", &max_fps, -1, 300);
		ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	}
	if( ImGui::CollapsingHeader("info") ) {
		ImGui::Text("#vert %d", cur_body->nr_verts);
		ImGui::Text("#ptcl %d", cur_body->nr_ptcls);
		ImGui::Text("#tris %d", cur_body->nr_tris);
		ImGui::Separator();
		ImGui::Text("#stretch %d", cur_body->c_stretchs.size());
		ImGui::Text("#shear %d", cur_body->c_shears.size());
		ImGui::Text("#dist_bend %d", cur_body->c_dist_bends.size());
		ImGui::Text("#dih_bend %d", cur_body->c_dih_bends.size());
		ImGui::Text("#iso_bend %d", cur_body->c_iso_bends.size());
	}
	if( ImGui::CollapsingHeader("compliance") ) {
		ImGui::SliderFloat("stretch", &cur_body->compliance.stretch, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("dist_bend", &cur_body->compliance.dist_bend, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("dih_bend", &cur_body->compliance.dih_bend, 0.f, 100.f, "%.6f");
		ImGui::SliderFloat("iso_bend", &cur_body->compliance.iso_bend, 0.f, 100.f, "%.6f");
		ImGui::Separator();
		ImGui::SliderFloat("presure", &cur_body->pressure, 0.f, 1000.f, "%.6f");
		ImGui::Separator();
		ImGui::SliderFloat("shear", &cur_body->compliance.shear, 0.f, 1.f, "%.6f");

		ImGui::SliderFloat("air_drag", &phy_scene.air_drag, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground friction", &c_ground.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground restitution", &c_ground.restitution, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere friction", &c_sphere.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere restitution", &c_sphere.restitution, 0.f, 1.f, "%.6f");
	}

	if( ImGui::CollapsingHeader("debug") ) {
		LimGui::PlotVal("dih_c", "");
		LimGui::PlotVal("dih_denom", "");
		LimGui::PlotVal("dih_e0Len", "");
		
		LimGui::PlotVal("dt", "ms", delta_time*1000.f);
		LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	}
	ImGui::End();



	// input handling
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
			picked_info.picked = false;
		}
	}
	if(ImGui::IsKeyPressed(ImGuiKey_B, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		ptcl_test.inv_body_mass = 1/0.1f;
		ptcl_test.addPtcl(vp.camera.pos, 1/0.1f, 10.f*mouseRay);
	}
	if(ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
		is_paused = !is_paused;
	}
}