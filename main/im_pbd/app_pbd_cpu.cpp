#include "app_pbd_cpu.h"
#include <limbrary/tools/limgui.h>
#include "pbd/pbd.h"
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/model.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/etc.h>
#include <limbrary/tools/s_asset_lib.h>
using namespace lim;
using namespace glm;


namespace {
	AppPbdCpu* g_app = nullptr;
	pbd::PhyScene phy_scene;
	pbd::SoftBody ptcl_test;
	pbd::SoftBody* cur_body = nullptr;
	pbd::ColliderPlane c_ground;
	pbd::ColliderSphere c_sphere;
	
	bool is_paused = true;
	float time_speed = 1.f;
	bool draw_mesh = false;
	bool enable_tex = true;
	bool draw_dpi_dir = false;
	bool update_nor_with_ptcl = true;
	
	int nr_ms_slices = 13;
	int nr_shear = 2;
	float size_scale = 0.7f;
	float body_mass = 1.f;
	bool fix_start = true;
	bool is_ptcl_ref_close_verts = false;

	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
	// turn false if you want vertex tri face normal

	const char* mesh_type_names[] = {
		"Bunny", "Cheems", "Buff",
		"Cube", "Shared Cube",
		"Sphere", "Ico Sphere",
		"Plane", "Cloth", "Donut",
	};
	const int nr_mesh_type_names = sizeof(mesh_type_names)/sizeof(mesh_type_names[0]);
	enum MeshType : int {
		MT_BUNNY, MT_CHEEMS, MT_BUFF,
		MT_CUBE, MT_SHARED_CUBE,
		MT_SPHERE, MT_ICO_SPHERE,
		MT_PLANE, MT_CLOTH, MT_DONUT, 
	};
	MeshType cur_mesh_type = MT_SPHERE;
}






static void resetApp() {
	c_sphere.tf.pos = {0,0.5f,0};
	c_sphere.tf.scale = vec3(0.3f);
	c_sphere.tf.update();

	c_ground.tf.scale = {50.f, 50.f, 1.f};
	c_ground.tf.ori = glim::rotateV(glim::right, -90.f);
	c_ground.tf.update();

	// make new soft body ==============================================
	pbd::SoftBody::ConstraintParams tempComp;
	if(cur_body) {
		tempComp = cur_body->params;
		delete cur_body;
	}
	Mesh* ms = nullptr;
	mat4 tf = mat4(1);

	switch(cur_mesh_type) {
		case MT_BUNNY: {
			Model md;
			md.importFromFile("assets/models/pbd_test/bunny.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);
			break;
		}
		case MT_CHEEMS: {
			Model md;
			md.importFromFile("assets/models/pbd_test/cheems.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);
			break;
		}
		case MT_BUFF: {
			Model md;
			md.importFromFile("assets/models/pbd_test/buff-doge.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);
			break;
		}
		case MT_CUBE: {
			nr_ms_slices = min(4, nr_ms_slices);
			ms = new MeshCube(1.f, true, true);
			ms->subdivide(nr_ms_slices);
			break;
		}
		case MT_SHARED_CUBE: {
			nr_ms_slices = min(4, nr_ms_slices);
			ms = new MeshCube(1.f, false, false);
			ms->subdivide(nr_ms_slices);
			break;
		}
		case MT_SPHERE: {
			int nrSlices = max(6, nr_ms_slices);
			int nrStacks = nrSlices/2;
			ms = new MeshSphere(1.f, nrSlices, nrStacks, true, true);
			break;
		}
		case MT_ICO_SPHERE: {
			nr_ms_slices = min(2, nr_ms_slices);
			ms = new MeshIcoSphere(1.f, nr_ms_slices, true, true);
			break;
		}
		case MT_PLANE: {
			nr_ms_slices = max(1, nr_ms_slices);
			ms = new MeshPlane(1.f, 1.f, nr_ms_slices, nr_ms_slices, true, true);
			break;
		}
		case MT_CLOTH: {
			nr_ms_slices = max(1, nr_ms_slices);
			ms = new MeshCloth(1.f, 1.f, nr_ms_slices, nr_ms_slices, true, true);
			break;
		}
		case MT_DONUT: {
			int nrSlices = max(10, nr_ms_slices);
			int nrRingVerts = nrSlices/2;
			ms = new MeshDonut(2.f, 0.7f, nrSlices, nrRingVerts, true, true);
			break;
		}
	}

	tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f) * scale(vec3(size_scale))* tf;
	for( vec3& p : ms->poss ) {
		p = vec3(tf*vec4(p,1));
	}
	cur_body = new pbd::SoftBody(std::move(*ms), nr_shear, bend_type, body_mass, is_ptcl_ref_close_verts);
	cur_body->initGL();
	cur_body->params = tempComp;
	if( fix_start ) {
		cur_body->w_s[0] = 0.f;
		if( cur_mesh_type==MT_PLANE||cur_mesh_type==MT_CLOTH ) {
			cur_body->w_s[nr_ms_slices] = 0.f;
		}
	}

	delete ms;

	// update scene ================================================
	
	is_paused = true;


	phy_scene.bodies.clear();
	phy_scene.bodies.push_back( cur_body );
	phy_scene.bodies.push_back( &ptcl_test );

	phy_scene.colliders.clear();
	phy_scene.colliders.push_back( &c_ground );
	phy_scene.colliders.push_back( &c_sphere );
}







AppPbdCpu::AppPbdCpu() : AppBaseCanvas3d(1200, 780, APP_NAME, false, 10, 10000, 100000)
{
	custom_max_fps = 60;
	prog_ms.attatch("mvp.vs").attatch("im_pbd/shaders/ndl_tex.fs").link();

	texture.s_wrap_param = GL_REPEAT;
	texture.initFromFile("assets/images/uv_grid.jpg", true);
	
	g_app = this;
	resetApp();
}
AppPbdCpu::~AppPbdCpu()
{
	safeDel(cur_body);
}
void AppPbdCpu::canvasUpdate()
{
	if( is_paused )
		return;
	phy_scene.update(delta_time*time_speed);
	if( draw_mesh ) {
		if( is_ptcl_ref_close_verts == false ) {
			cur_body->updateNorsAndUpload();
		}
		else if( update_nor_with_ptcl ) {
			cur_body->updatePossAndNorsWithPtclAndUpload();
		}
		else {
			cur_body->updatePossAndNorsWithVertAndUpload();
		}
	}

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
	// glEnable(GL_CULL_FACE);

	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));
	prog_ms.setUniform("enable_Tex", enable_tex);
	prog_ms.setTexture("tex", texture.tex_id);

	// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	// glDisable(GL_CULL_FACE);

}



static void drawBody(const pbd::SoftBody& body) {
	for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawSphere(body.x_s[i], 0.05f, (body.w_s[i]>0)?vec3{1,1,0}:vec3{1,0,0});
	}
	for( const auto& c : body.c_stretchs ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], 0.025f, {1,1,0} );
		// if( draw_dpi_dir ) for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_shears ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], 0.025f, {0,1,1} );
		// for( int i=0; i<2; i++ ) {
		// 	g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i]*0.1f, {0,0,1} );
		// }
	}
	for( const auto& c : body.c_dist_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], 0.025f, {0.7,0.1,0}, 0.03f );
		if( draw_dpi_dir ) for( int i=0; i<2; i++ ) {
			g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i], 0.025f, {0,0,1} );
		}
	}
	for( const auto& c : body.c_dih_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], 0.025f, {0,0,1}, 0.027f );
	}
	for( const auto& c : body.c_iso_bends ) {
		g_app->drawCylinder( body.x_s[c.idx_ps.x], body.x_s[c.idx_ps.y], 0.025f, {0,0,1}, 0.027f );
		if( draw_dpi_dir ) for( int i=0; i<4; i++ ) {
			g_app->drawCylinder( body.x_s[c.idx_ps[i]], body.x_s[c.idx_ps[i]]+c.dPi[i], 0.025f, {0,0,1} );
		}
	}
	for( const auto& c : body.c_points ) {
		g_app->drawSphere( c.point, 0.05f, {1,0,0} );
		g_app->drawCylinder( c.point, body.x_s[c.idx_p], 0.025f, {0,0,1} );
	}
	if( draw_dpi_dir ) for( int i=0; i<body.nr_ptcls; i++ ) {
		g_app->drawCylinder( body.x_s[i], body.x_s[i]+body.debug_dirs[i], 0.025f, {0,0,1} );
	}
}
void AppPbdCpu::canvasDraw() const
{
	if( !draw_mesh ) {
		for( auto b : phy_scene.bodies ) {
			drawBody( *b );
		}
	}

	drawSphere(c_sphere.tf.mtx, {1,0,0});

	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, 0.025f, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, 0.025f, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, 0.025f, {0,0,1});

	drawSphere(c_sphere.tf.mtx, {1,0,0});
	drawQuad(c_ground.tf.mtx, {0,0.3f,0});
}



namespace
{
	struct PickedPtclInfo {
		bool picked = false;
		pbd::SoftBody* body;
		int ptcl_idx;
		vec3& x() {
			return body->x_s[ptcl_idx];
		}
		vec3& v() {
			return body->v_s[ptcl_idx];
		}
		float& w() {
			return body->w_s[ptcl_idx];
		}
	};
	PickedPtclInfo picked_ptcl_info;


	struct PickedTriInfo {
		bool picked = false;
		pbd::SoftBody* body;
		ivec3 tri;
		vec3 point;
		void setPointConstraints() {
			body->c_points.push_back(
				pbd::ConstraintPoint(*body, tri.x, point)
			);
			body->c_points.push_back(
				pbd::ConstraintPoint(*body, tri.y, point)
			);
			body->c_points.push_back(
				pbd::ConstraintPoint(*body, tri.z, point)
			);
		}
		void updatePointConstraints() {
			for(auto& c : body->c_points) {
				c.point = point;
			}
		}
		void delPointConstraints() {
			body->c_points.clear();
		}
	};
	PickedTriInfo picked_tri_info;
}

static void pickPtcl( const vec3& rayOri, const vec3& rayDir ) {
	float minDepth = FLT_MAX;
	picked_ptcl_info.picked = false;

	for(pbd::SoftBody* body : phy_scene.bodies) {
		for(int i=0; i<body->nr_ptcls; i++) {
			vec3 toObj = body->x_s[i] - rayOri;
			float distFromLine = glm::length( glm::cross(rayDir, toObj) );
			if( distFromLine < 0.04f ) {
				float distProjLine = glm::dot(rayDir, toObj);
				if( distProjLine>0 && distProjLine<minDepth ) {
					minDepth = distProjLine;
					picked_ptcl_info.picked = true;
					picked_ptcl_info.body = body;
					picked_ptcl_info.ptcl_idx = i;
				}
			}
		}
	}
}
static void pickTriangle( const vec3& rayDir, const vec3& rayOri ) {
	float minDepth = FLT_MAX;
	const float searchDepth = 50.f;
	picked_tri_info.picked = false;

	for(pbd::SoftBody* body : phy_scene.bodies) {
		for(int i=0; i<body->nr_tris; i++) {
			ivec3& tri = body->ptcl_tris[i]; 
			vec3& t1 = body->x_s[tri.x];
			vec3& t2 = body->x_s[tri.y];
			vec3& t3 = body->x_s[tri.z];
			
			float depth = glim::intersectTriAndRayBothFaces(rayOri, rayDir, t1, t2, t3);
			if( depth<0 || depth>searchDepth || depth>minDepth )
				continue;
			minDepth = depth;
			picked_tri_info.picked = true;
			picked_tri_info.body = body;
			picked_tri_info.tri = tri;
			picked_tri_info.point = depth*rayDir + rayOri;
		}
	}
}
void AppPbdCpu::canvasImGui()
{
	ImGui::Begin("pbd ctrl");
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Text("you can remake mesh when paused");
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
			bool needReset = false;
			needReset |= ImGui::Combo("mesh type", (int*)&cur_mesh_type, mesh_type_names, nr_mesh_type_names);
			needReset |= ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);
			needReset |= ImGui::SliderFloat("size scale", &size_scale, 0.5f, 3.f);
			needReset |= ImGui::SliderInt("nr ms slices", &nr_ms_slices, 0, 20);
			needReset |= ImGui::SliderInt("nr shears", &nr_shear, 0, 2);
			needReset |= ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4);
			needReset |= ImGui::Checkbox("fix", &fix_start);
			needReset |= ImGui::SliderFloat("body mass", &body_mass, 0.01f, 2.f);
			if( needReset ) {
				resetApp();
			}
		}
		if( ImGui::Button("reset") ) {
			resetApp();
		}
	}
	
	if( ImGui::CollapsingHeader("draw&run") ) {
		ImGui::Checkbox("draw mesh", &draw_mesh);
		ImGui::Checkbox("enable texture", &enable_tex);
		if( is_ptcl_ref_close_verts ) {
			ImGui::Checkbox("make nor with ptcl tris", &update_nor_with_ptcl);
		}
		ImGui::Checkbox("draw dPi dir draw", &draw_dpi_dir);
		ImGui::SliderInt("# steps", &cur_body->nr_steps, 1, 50);
		ImGui::SliderInt("max fps", &custom_max_fps, 20, 300);
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
	if( ImGui::CollapsingHeader("params") ) {
		ImGui::SliderFloat("point", &cur_body->params.inv_stiff_point, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("distance", &cur_body->params.inv_stiff_dist, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("stretch_pct", &cur_body->params.stretch_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("shear_pct", &cur_body->params.shear_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("bend_pct", &cur_body->params.bend_pct, 0.f, 1.0f, "%.6f");
		ImGui::Separator();
		ImGui::SliderFloat("dih_bend", &cur_body->params.inv_stiff_dih_bend, 0.f, 100.f, "%.6f");
		ImGui::SliderFloat("iso_bend", &cur_body->params.inv_stiff_iso_bend, 0.f, 100.f, "%.6f");
		
		ImGui::Separator();
		ImGui::Text("volume ori: %.4f", cur_body->c_volume.ori_six_volume);
		ImGui::Text("volume cur: %.4f", cur_body->c_volume.cur_six_volume);
		ImGui::SliderFloat("volume", &cur_body->params.inv_stiff_volume, 0.f, 10.f, "%.6f");
		ImGui::SliderFloat("pressure", &cur_body->params.pressure, 0.f, 20.f, "%.6f");
		// ImGui::SliderFloat("pressure", &cur_body->params.pressure, 0.f, 1.f, "%.6f");

		ImGui::Separator();
		ImGui::SliderFloat("air_drag", &phy_scene.air_drag, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground friction", &c_ground.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground restitution", &c_ground.restitution, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere friction", &c_sphere.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere restitution", &c_sphere.restitution, 0.f, 1.f, "%.6f");
	}

	if( ImGui::CollapsingHeader("debug") ) {
		LimGui::PlotVal("dt", "ms", delta_time*1000.f);
		LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	}
	ImGui::End();



	// input handling
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Left, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickPtcl(vp.camera.pos, mouseRay);
		if( picked_ptcl_info.picked ) {
			vp.camera.enabled = false;
			picked_ptcl_info.w() = 0.f;
			picked_ptcl_info.v() = vec3(0.f);
		}
	}
	else if(picked_ptcl_info.picked && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		vec3 target = picked_ptcl_info.x();
		vp.movePosFormMouse(target);
		picked_ptcl_info.x() = target;
	}
	else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		vp.camera.enabled = true;
		picked_ptcl_info.picked = false;
	}
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickPtcl(vp.camera.pos, mouseRay);
		if( picked_ptcl_info.picked ) {
			picked_ptcl_info.w() = picked_ptcl_info.body->inv_ptcl_mass;
			picked_ptcl_info.picked = false;
		}
	}



	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickTriangle(mouseRay, vp.camera.pos);
		if( picked_tri_info.picked ) {
			vp.camera.enabled = false;
			picked_tri_info.setPointConstraints();
		}
	}
	else if(picked_tri_info.picked && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		vp.movePosFormMouse(picked_tri_info.point);
		picked_tri_info.updatePointConstraints();
	}
	else if(ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		vp.camera.enabled = true;
		if( picked_tri_info.picked ) {
			picked_tri_info.delPointConstraints();
			picked_tri_info.picked = false;
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
	if( ImGui::IsKeyPressed(ImGuiKey_R) ) {
		resetApp();
	}
}