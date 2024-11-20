#include "app_pbd_gpu.h"
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/gl.h>
#include <limbrary/tools/etc.h>

#include <glm/gtx/transform.hpp>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

namespace {
	AppPbdGpu* g_app = nullptr;
	pbd::SoftBodyGpu* cur_body = nullptr;
	pbd::ColliderPlane c_ground;
	pbd::ColliderSphere c_sphere;


	bool is_paused = true;
	bool enable_tex = true;
	bool draw_edges = false;
	float time_speed = 1.f;
	bool update_nor_with_ptcl = true;


	int nr_shear = 2;
	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
	float body_mass = 1.f;
	bool is_ptcl_ref_close_verts = true;

	int nr_ms_slices = 14;
	float size_scale = 0.7f;
	bool fix_start = true;


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
	MeshType cur_mesh_type = MT_PLANE;
}



static void resetApp() {
	// make new soft body ==================================================
	pbd::SoftBody::ConstraintParams tempComp;
	if(cur_body) {
		tempComp = cur_body->params;
		delete cur_body;
	}
	Mesh* ms = nullptr;
	mat4 tf = mat4(1);

	switch(cur_mesh_type) {
		case MT_BUNNY: {
			ModelData md;
			md.importFromFile("assets/models/pbd_test/bunny.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);
			break;
		}
		case MT_CHEEMS: {
			ModelData md;
			md.importFromFile("assets/models/dwarf/Dwarf_2_Very_Low.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);

			break;
		}
		case MT_BUFF: {
			ModelData md;
			md.importFromFile("assets/models/pbd_test/buff-doge.obj", false, true, 1.f, vec3(0));
			ms = md.own_meshes[0].raw;
			md.own_meshes[0] = nullptr;
			tf = md.getLocalToMeshMtx(ms);
			break;
		}
		case MT_CUBE: {
			nr_ms_slices = glm::min(4, nr_ms_slices);
			ms = new MeshCube(1.f, true, true);
			ms->subdivide(nr_ms_slices);
			break;
		}
		case MT_SHARED_CUBE: {
			nr_ms_slices = glm::min(4, nr_ms_slices);
			ms = new MeshCube(1.f, false, false);
			ms->subdivide(nr_ms_slices);
			break;
		}
		case MT_SPHERE: {
			int nrSlices = glm::max(6, nr_ms_slices);
			int nrStacks = nrSlices/2;
			ms = new MeshSphere(1.f, nrSlices, nrStacks, true, true);
			break;
		}
		case MT_ICO_SPHERE: {
			nr_ms_slices = glm::min(2, nr_ms_slices);
			ms = new MeshIcoSphere(1.f, nr_ms_slices, true, true);
			break;
		}
		case MT_PLANE: {
			nr_ms_slices = glm::max(1, nr_ms_slices);
			ms = new MeshPlane(1.f, 1.f, nr_ms_slices, nr_ms_slices, true, true);
			break;
		}
		case MT_CLOTH: {
			nr_ms_slices = glm::max(1, nr_ms_slices);
			ms = new MeshCloth(1.f, 1.f, nr_ms_slices, nr_ms_slices, true, true);
			break;
		}
		case MT_DONUT: {
			int nrSlices = glm::max(10, nr_ms_slices);
			int nrRingVerts = nrSlices/2;
			ms = new MeshDonut(2.f, 0.7f, nrSlices, nrRingVerts, true, true);
			break;
		}
	}

	tf = glm::translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f) * glm::scale(vec3(size_scale))* tf;
	
	for( vec3& p : ms->poss ) {
		p = vec3(tf*vec4(p,1));
	}
	cur_body = new pbd::SoftBodyGpu(std::move(*ms), nr_shear, bend_type, body_mass, is_ptcl_ref_close_verts);
	cur_body->params = tempComp;
	if( fix_start ) {
		cur_body->w_s[0] = 0.f;
		if( cur_mesh_type==MT_PLANE||cur_mesh_type==MT_CLOTH ) {
			cur_body->w_s[nr_ms_slices] = 0.f;
		}
	}
	cur_body->initGL();

	delete ms;

	// update scene ================================================

	is_paused = true;

	g_app->phy_scene.bodies.clear();
	g_app->phy_scene.bodies.push_back( cur_body );

	g_app->phy_scene.colliders.clear();
	g_app->phy_scene.colliders.push_back( &c_ground );
	g_app->phy_scene.colliders.push_back( &c_sphere );
}


AppPbdGpu::AppPbdGpu() : AppBaseCanvas3d(1200, 780, APP_NAME, false, 10, 100, 100)
{
	custom_max_fps = 1000;
	prog_ms.attatch("mvp.vs").attatch("im_pbd/shaders/ndl_tex.fs").link();

	texture.s_wrap_param = GL_REPEAT;
	texture.initFromFile("assets/images/uv_grid.jpg", true);

	int max_ssbo_bindings;
	glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &max_ssbo_bindings);
	log::pure("GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS: %d\n", max_ssbo_bindings);
	
	g_app = this;
	resetApp();
}
AppPbdGpu::~AppPbdGpu()
{
	safeDel(cur_body);
}

void AppPbdGpu::canvasUpdate()
{
	if( is_paused )
		return;
	phy_scene.update(delta_time*time_speed);
}


void AppPbdGpu::customDrawShadow(const mat4& mtx_View, const mat4& mtx_Proj) const
{
	const Program& sProg = lim::asset_lib::prog_shadow_static->use();
	sProg.setUniform("mtx_View", mtx_View);
	sProg.setUniform("mtx_Proj", mtx_Proj);
	sProg.setUniform("mtx_Model", glm::mat4(1));

	if( draw_edges ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		cur_body->bindAndDrawGL();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else {
		cur_body->bindAndDrawGL();
	}
}
void AppPbdGpu::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));
	prog_ms.setUniform("enable_Tex", enable_tex);
	prog_ms.setTexture("tex", texture.tex_id);

	if( draw_edges ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		cur_body->bindAndDrawGL();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else {
		cur_body->bindAndDrawGL();
	}
}

void AppPbdGpu::canvasDraw() const
{
	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, 0.025f, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, 0.025f, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, 0.025f, {0,0,1});

	drawQuad(vec3{0}, {0,1,0}, vec2{100,100}, {0,0.3f,0});
}





namespace
{
	int picked_ptcl_idx = -1;
	int picked_tri_idx = -1;
	vec3 picked_tri_pos;
}

static void pickPtcl( const vec3& rayOri, const vec3& rayDir ) {
	float minDepth = FLT_MAX;
	picked_ptcl_idx = -1;

	cur_body->downloadXs();

	for(int i=0; i<cur_body->nr_ptcls; i++) {
		vec3 toObj = cur_body->x_s[i] - rayOri;
		float distFromLine = glm::length( glm::cross(rayDir, toObj) );
		if( distFromLine < 0.02f ) {
			float distProjLine = glm::dot(rayDir, toObj);
			if( distProjLine>0 && distProjLine<minDepth ) {
				minDepth = distProjLine;
				picked_ptcl_idx = i;
			}
		}
	}
}
static void pickTriangle( const vec3& rayDir, const vec3& rayOri ) {
	const float searchDepth = 50.f;
	float minDepth = FLT_MAX;
	picked_tri_idx = -1;

	cur_body->downloadXs();

	for(int i=0; i<cur_body->nr_tris; i++) {
		ivec3& tri = cur_body->ptcl_tris[i]; 
		vec3& t1 = cur_body->x_s[tri.x];
		vec3& t2 = cur_body->x_s[tri.y];
		vec3& t3 = cur_body->x_s[tri.z];
		
		float depth = glim::intersectTriAndRayBothFaces(rayOri, rayDir, t1, t2, t3);
		if( depth<0 || depth>searchDepth || depth>minDepth )
			continue;
		minDepth = depth;
		picked_tri_idx = i;
		picked_tri_pos = depth*rayDir + rayOri;
	}
}



void AppPbdGpu::canvasImGui()
{
	ImGui::Begin("pbd ctrl##gpu");
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Text("you can remake mesh when paused");
		ImGui::Checkbox("pause", &is_paused);
		if( is_paused ) {
			bool needReset = false;
			needReset |= ImGui::Combo("mesh type", (int*)&cur_mesh_type, mesh_type_names, nr_mesh_type_names);
			needReset |= ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4);
			needReset |= ImGui::SliderInt("nr ms slices", &nr_ms_slices, 0, 30);
			needReset |= ImGui::SliderInt("nr shears", &nr_shear, 0, 2);
			needReset |= ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);
			needReset |= ImGui::SliderFloat("body mass", &body_mass, 0.01f, 2.f);
			needReset |= ImGui::Checkbox("fix start", &fix_start);
			if( needReset ) {
				resetApp();
			}
		}
		if( ImGui::Button("reset") ) {
			resetApp();
		}

		ImGui::Separator();

		if( is_ptcl_ref_close_verts ) {
			ImGui::Checkbox("make nor with ptcl tris", &update_nor_with_ptcl);
		}

		ImGui::Checkbox("enable tex", &enable_tex);
		ImGui::Checkbox("draw edges", &draw_edges);
		ImGui::SliderInt("# steps", &cur_body->nr_steps, 1, 50);
		ImGui::SliderInt("max fps", &custom_max_fps, 20, 1000);
		ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	}

	if( ImGui::CollapsingHeader("info") ) {
		ImGui::Text("subDt: %.10f", delta_time/cur_body->nr_steps);
		ImGui::Text("max #step: %d", int(delta_time/0.0001f)); // if subDt is under 0.0001s, verlet integral get error
		ImGui::Text("#thread*#threadgroup %d*%d", cur_body->nr_threads, cur_body->nr_thread_groups_by_ptcls);
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
		ImGui::SliderFloat("gravity", &phy_scene.G.y, 0.f, -15.f);
		ImGui::SliderFloat("point", &cur_body->params.inv_stiff_point, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("distance", &cur_body->params.inv_stiff_dist, 0.0001f, 1.0f, "%.6f");
		ImGui::SliderFloat("stretch_pct", &cur_body->params.stretch_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("shear_pct", &cur_body->params.shear_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("bend_pct", &cur_body->params.bend_pct, 0.f, 1.0f, "%.6f");

		ImGui::SliderFloat("dih_bend", &cur_body->params.inv_stiff_dih_bend, 0.f, 100.f, "%.6f");

		ImGui::Text("volume cur: %.4f", cur_body->c_volume.cur_six_volume);
		ImGui::SliderFloat("pressure", &cur_body->params.pressure, 0.f, 100.f, "%.6f");


		ImGui::Separator();
		ImGui::SliderFloat("air_drag", &phy_scene.air_drag, 0.f, 3.f, "%.6f");
		ImGui::SliderFloat("ground friction", &c_ground.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("ground restitution", &c_ground.restitution, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere friction", &c_sphere.friction, 0.f, 1.f, "%.6f");
		ImGui::SliderFloat("sphere restitution", &c_sphere.restitution, 0.f, 1.f, "%.6f");
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("debug") ) {
		LimGui::PlotVal("dt", "ms", delta_time*1000.f);
		LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	}
	ImGui::End();



	// input handling
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Left, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickPtcl(vp.camera.pos, mouseRay);
		if( picked_ptcl_idx >= 0 ) {
			vp.camera.enabled = false;
			float invInfMass = 0.f;
			cur_body->w_s[picked_ptcl_idx] = invInfMass;
			glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_w_s);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*picked_ptcl_idx, sizeof(float), &invInfMass);
		}
	}
	else if(picked_ptcl_idx >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		vec3 target = cur_body->x_s[picked_ptcl_idx];
		vp.movePosFormMouse(target);
		cur_body->x_s[picked_ptcl_idx] = target;
		glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_x_s);
		glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3)*picked_ptcl_idx, sizeof(vec3), &target);
	}
	else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		vp.camera.enabled = true;
		picked_ptcl_idx = -1;
	}
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickPtcl(vp.camera.pos, mouseRay);
		if( picked_ptcl_idx>=0 ) {
			float invPtclMass = cur_body->inv_ptcl_mass;
			cur_body->w_s[picked_ptcl_idx] = invPtclMass;
			glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_w_s);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*picked_ptcl_idx, sizeof(float), &invPtclMass);
			picked_ptcl_idx = -1;
		}
	}

	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
		const vec3 mouseRay = vp.getMousePosRayDir();
		pickTriangle(mouseRay, vp.camera.pos);
		if( picked_tri_idx>=0) {
			vp.camera.enabled = false;
			ivec3 tri = cur_body->ptcl_tris[picked_tri_idx];
			cur_body->c_points.clear();
			cur_body->c_points.push_back(
				pbd::ConstraintPoint(*cur_body, tri.x, picked_tri_pos)
			);
			cur_body->c_points.push_back(
				pbd::ConstraintPoint(*cur_body, tri.y, picked_tri_pos)
			);
			cur_body->c_points.push_back(
				pbd::ConstraintPoint(*cur_body, tri.z, picked_tri_pos)
			);
			assert(cur_body->buf_c_points==0);
			glGenBuffers(1, &cur_body->buf_c_points);
			glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_c_points);
			glBufferData(GL_ARRAY_BUFFER, sizeof(pbd::ConstraintPoint)*3, cur_body->c_points.data(), GL_DYNAMIC_DRAW);
		}
	}
	else if(picked_tri_idx>=0 && ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
		vp.movePosFormMouse(picked_tri_pos);
		for( auto& c : cur_body->c_points ) {
			c.point = picked_tri_pos;
		}
		glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_c_points);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pbd::ConstraintPoint)*3, cur_body->c_points.data());
	}
	else if(picked_tri_idx>=0 && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
		vp.camera.enabled = true;
		picked_tri_idx = -1;
		gl::safeDelBufs(&cur_body->buf_c_points);
	}


	if( ImGui::IsKeyPressed(ImGuiKey_F) ) {
		std::fill(cur_body->w_s.begin(), cur_body->w_s.end(), cur_body->inv_ptcl_mass);
		glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_w_s);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*cur_body->nr_ptcls, cur_body->w_s.data());
	}
	if( ImGui::IsKeyPressed(ImGuiKey_Space) ) {
		is_paused = !is_paused;
	}
	if( ImGui::IsKeyPressed(ImGuiKey_R) ) {
		resetApp();
	}
}