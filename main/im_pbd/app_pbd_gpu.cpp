#include "app_pbd_gpu.h"
#include <limbrary/limgui.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace glm;

namespace {
	AppPbdGpu* g_app = nullptr;
	pbd::SoftBodyGpu* cur_body = nullptr;
	pbd::ColliderPlane c_ground;
	pbd::ColliderSphere c_sphere({0,0.5f,0}, 0.3f);


	bool is_paused = true;
	bool enable_tex = true;;
	float time_speed = 1.f;
	bool update_nor_with_ptcl = true;


	int nr_ms_slices = 14;
	int nr_shear = 2;
	float size_scale = 0.7f;
	float body_mass = 1.f;
	bool fix_start = true;
	bool is_ptcl_ref_close_verts = false;


	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
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
		case MT_PLANE: {
			nr_ms_slices = max(1, nr_ms_slices);
			ms = new MeshCloth(1.f, 1.f, nr_ms_slices, nr_ms_slices, true, true);
			break;
		}
	}

	// tf = translate(vec3(0,2,0)) * glim::rotateX(glim::pi90*0.1f)* glim::rotateY(glim::pi90*0.2f) * scale(vec3(size_scale))* tf;
	tf = translate(vec3(0,2.5f,0)) * glim::rotateZ(glim::pi90*0.6f) * scale(vec3(size_scale))* tf;
	// tf = translate(vec3(0,2,0)) * scale(vec3(size_scale))* tf;
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
	max_fps = 1000;
	prog_ms.attatch("mvp.vs").attatch("im_pbd/shaders/ndl_tex.fs").link();

	texture.s_wrap_param = GL_REPEAT;
	texture.initFromFile("assets/images/uv_grid.jpg", true);
	
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
	Program& sProg = lim::AssetLib::get().prog_shadow_static;
	sProg.use();
	sProg.setUniform("mtx_View", mtx_View);
	sProg.setUniform("mtx_Proj", mtx_Proj);
	sProg.setUniform("mtx_Model", glm::mat4(1));

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
void AppPbdGpu::customDraw(const Camera& cam, const LightDirectional& lit) const 
{
	prog_ms.use();
	cam.setUniformTo(prog_ms);
	lit.setUniformTo(prog_ms);
	prog_ms.setUniform("mtx_Model", glm::mat4(1));
	prog_ms.setUniform("enable_Tex", enable_tex);
	prog_ms.setTexture("tex", texture.tex_id);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	cur_body->bindAndDrawGL();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void AppPbdGpu::canvasDraw() const
{
	// basis object 10cm
	drawCylinder({0,0,0}, {0.1f,0,0}, {1,0,0});
	drawCylinder({0,0,0}, {0,0.1f,0}, {0,1,0});
	drawCylinder({0,0,0}, {0,0,0.1f}, {0,0,1});

	drawQuad(vec3{0}, {0,1,0}, {0,0.3f,0});
}

void AppPbdGpu::canvasImGui()
{
	ImGui::Begin("pbd ctrl");
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Text("you can remake mesh when paused");
		ImGui::Checkbox("pause", &is_paused);
		if( is_paused ) {
			bool needReset = false;
			needReset |= ImGui::SliderInt("nr ms slices", &nr_ms_slices, 0, 100);
			needReset |= ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);
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

		ImGui::Checkbox("enable_tex", &enable_tex);
		ImGui::SliderInt("# steps", &cur_body->nr_steps, 1, 50);
		ImGui::SliderInt("max fps", &max_fps, 20, 1000);
		ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	}

	if( ImGui::CollapsingHeader("info") ) {
		ImGui::Text("subDt: %.10f", delta_time/cur_body->nr_steps);
		ImGui::Text("max #step: %d", int(delta_time/0.0001f)); // if subDt is under 0.0001s, verlet integral get error
		ImGui::Text("#thread*#threadgroup %d*%d", cur_body->nr_threads, cur_body->nr_thread_groups);
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
		ImGui::SliderFloat("distance", &cur_body->params.inv_stiff_dist, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("stretch_pct", &cur_body->params.stretch_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("shear_pct", &cur_body->params.shear_pct, 0.f, 1.0f, "%.6f");
		ImGui::SliderFloat("bend_pct", &cur_body->params.bend_pct, 0.f, 1.0f, "%.6f");
	}

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("debug") ) {
		LimGui::PlotVal("dt", "ms", delta_time*1000.f);
		LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	}
	ImGui::End();


	if( ImGui::IsKeyPressed(ImGuiKey_Space) ) {
		is_paused = !is_paused;
	}
	if( ImGui::IsKeyPressed(ImGuiKey_R) ) {
		resetApp();
	}
}