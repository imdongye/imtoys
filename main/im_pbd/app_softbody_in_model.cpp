#include "app_softbody_in_model.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/gl.h>
#include <functional>
#include "pbd/pbd.h"

using namespace lim;
using namespace glm;


namespace {
	float time_speed = 1.f;
	bool is_paused = true;
	bool draw_edges = false;

	int nr_shear = 2;
	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
	float body_mass = 1.f;
	bool is_ptcl_ref_close_verts = true;


	const Mesh* src_mesh = nullptr; // todo recreate soft body
	pbd::SoftBodyGpu* cur_body = nullptr;
}


void AppSoftbodyInModel::reloadModel(const char* path) {
	src_mesh = nullptr;

	ModelData* srcMd = new ModelData();
	srcMd->importFromFile(path, true, true);
	for(auto& mat : srcMd->own_materials) {
		mat->Roughness = 0.519f;
		mat->Metalness = 0.73f;
	}
	srcMd->setProgToAllMat(&prog_skinned);

	scene.own_mds[0] = srcMd; // delete and change in OwnPtr
}
AppSoftbodyInModel::AppSoftbodyInModel() : AppBase(1480, 780, APP_NAME, false)
	, viewport(new FramebufferMs())
	, ib_light("assets/images/ibls/artist_workshop_4k.hdr")
{

	prog_skinned.name = "skeletal prog";
	prog_skinned.attatch("mvp_skinned_shadow.vs").attatch("im_pbd/shaders/brdf.fs").link();

	prog_static.name = "static prog";
	prog_static.attatch("mvp_shadow.vs").attatch("im_pbd/shaders/brdf.fs").link();


	scene.ib_light = &ib_light;
	scene.is_draw_env_map = true;
	scene.idx_LitMod = 0;
	scene.addOwn(new LightDirectional());
	scene.own_dir_lits.back()->setShadowEnabled(true);


	scene.own_mds.resize(2);

	scene.own_mds[0] = nullptr;
	reloadModel("assets/models/my/color.fbx");


	// floor
	{
		ModelData* floorMd = new ModelData();
		Material* flMat = floorMd->addOwn(new Material());
		floorMd->name = "floor";
		floorMd->root.ms = floorMd->addOwn(new MeshPlane(2.f, 2.f));
		floorMd->root.mat = flMat;
		floorMd->own_meshes.back()->initGL(true);
		flMat->prog = &prog_static;
		floorMd->root.tf.scale = glm::vec3(5.f);
		floorMd->root.tf.update();
		floorMd->root.updateGlobalTransform();

		Texture* flTex;
		flTex = floorMd->addOwn(new Texture());
		flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
		flTex->initFromFile("assets/images/floor/wood_floor_deck_diff_4k.jpg", true);
		flMat->map_Flags |= Material::MF_COLOR_BASE;
		flMat->map_ColorBase = flTex;

		flTex = floorMd->addOwn(new Texture());
		flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
		flTex->initFromFile("assets/images/floor/wood_floor_deck_arm_4k.jpg");
		flMat->map_Flags |= Material::MF_ARM;
		flMat->map_Roughness = flTex;

		flTex = floorMd->addOwn(new Texture());
		flTex->min_filter = GL_LINEAR_MIPMAP_LINEAR;
		flTex->initFromFile("assets/images/floor/wood_floor_deck_nor_gl_4k.jpg");
		flMat->map_Flags |= Material::MF_NOR;
		flMat->map_Bump = flTex;

		scene.own_mds[1] = floorMd;
	}


	viewport.camera.moveShift(glm::vec3(0,1.4f,0));
	viewport.camera.updateViewMtx();

	dnd_callbacks[this] = [this](int count, const char **paths) {
		reloadModel(paths[0]);
	};
}
AppSoftbodyInModel::~AppSoftbodyInModel()
{
}

void AppSoftbodyInModel::update() 
{
	if( !is_paused ) {
		phy_scene.update(time_speed*delta_time);
	}


	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if( draw_edges ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	scene.render(viewport.getFb(), viewport.camera);
	if( draw_edges ) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
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







void AppSoftbodyInModel::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGuiAndUpdateCam();


	ModelView& md = *scene.own_mds[0];
	LimGui::ModelViewEditor(md);
	

	ImGui::Begin("Scene");
	ImGui::Checkbox("draw envMap", &scene.is_draw_env_map);
	ImGui::Combo("light mode", &scene.idx_LitMod, "point\0ibl");
	ImGui::Checkbox("draw edges", &draw_edges);
	ImGui::End();


	ImGui::Begin("pbd ctrl##sbinmd");

	RdNode* pickedNd = LimGui::getPickedRenderNode();
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("soft body replacer") && pickedNd && pickedNd->ms && !pickedNd->is_local_is_global ) {
		ImGui::Text("%s mesh selected", pickedNd->ms->name.c_str());
		ImGui::SliderInt("nr shears", &nr_shear, 0, 2);
		ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4);
		ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);

		if( ImGui::Button("replace") ) {
			cur_body = pbd::replaceMeshInModelToSoftBody(md, *pickedNd
				, nr_shear, bend_type, body_mass, is_ptcl_ref_close_verts);
			Material* noSkinnedMat = new Material(*pickedNd->mat);
			noSkinnedMat->name = noSkinnedMat->name + "-noSkinned";
			noSkinnedMat->prog = &prog_static;
			md.src_md->addOwn(noSkinnedMat);
			pickedNd->mat = noSkinnedMat;

			// phy_scene.bodies.clear();
			phy_scene.bodies.push_back(cur_body);
		}
	}
	else {
		ImGui::Text("not selected");
		if( pickedNd && pickedNd->is_local_is_global ) {
			cur_body = (pbd::SoftBodyGpu*)pickedNd->ms;
		}
	}


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Checkbox("pause", &is_paused);
		if( ImGui::TreeNode("framerate") ) {
			LimGui::PlotVal("dt", "ms", delta_time*1000.f);
			LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
			ImGui::TreePop();
		}
		ImGui::SliderInt("max fps", &custom_max_fps, 20, 1000);
		ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	}


	if( cur_body ) {
		if( ImGui::CollapsingHeader("info") ) {
			ImGui::Text("name: %s", cur_body->name.c_str());
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
			ImGui::SliderInt("# steps", &cur_body->nr_steps, 1, 50);
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
			// ImGui::SliderFloat("ground friction", &c_ground.friction, 0.f, 1.f, "%.6f");
			// ImGui::SliderFloat("ground restitution", &c_ground.restitution, 0.f, 1.f, "%.6f");
			// ImGui::SliderFloat("sphere friction", &c_sphere.friction, 0.f, 1.f, "%.6f");
			// ImGui::SliderFloat("sphere restitution", &c_sphere.restitution, 0.f, 1.f, "%.6f");
		}
	}
	ImGui::End();





	// input handling
	if( cur_body )
	{
		if(ImGui::IsMouseClicked(ImGuiMouseButton_Left, false)) {
			const vec3 mouseRay = viewport.getMousePosRayDir();
			pickPtcl(viewport.camera.pos, mouseRay);
			if( picked_ptcl_idx >= 0 ) {
				viewport.camera.enabled = false;
				float invInfMass = 0.f;
				cur_body->w_s[picked_ptcl_idx] = invInfMass;
				glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_w_s);
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*picked_ptcl_idx, sizeof(float), &invInfMass);
			}
		}
		else if(picked_ptcl_idx >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			vec3 target = cur_body->x_s[picked_ptcl_idx];
			viewport.movePosFormMouse(target);
			cur_body->x_s[picked_ptcl_idx] = target;
			glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_x_s);
			glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3)*picked_ptcl_idx, sizeof(vec3), &target);
		}
		else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			viewport.camera.enabled = true;
			picked_ptcl_idx = -1;
		}
		if(ImGui::IsMouseClicked(ImGuiMouseButton_Middle, false)) {
			const vec3 mouseRay = viewport.getMousePosRayDir();
			pickPtcl(viewport.camera.pos, mouseRay);
			if( picked_ptcl_idx>=0 ) {
				float invPtclMass = cur_body->inv_ptcl_mass;
				cur_body->w_s[picked_ptcl_idx] = invPtclMass;
				glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_w_s);
				glBufferSubData(GL_ARRAY_BUFFER, sizeof(float)*picked_ptcl_idx, sizeof(float), &invPtclMass);
				picked_ptcl_idx = -1;
			}
		}

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Right, false)) {
			const vec3 mouseRay = viewport.getMousePosRayDir();
			pickTriangle(mouseRay, viewport.camera.pos);
			if( picked_tri_idx>=0) {
				viewport.camera.enabled = false;
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
			viewport.movePosFormMouse(picked_tri_pos);
			for( auto& c : cur_body->c_points ) {
				c.point = picked_tri_pos;
			}
			glBindBuffer(GL_ARRAY_BUFFER, cur_body->buf_c_points);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(pbd::ConstraintPoint)*3, cur_body->c_points.data());
		}
		else if(picked_tri_idx>=0 && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
			viewport.camera.enabled = true;
			picked_tri_idx = -1;
			gl::safeDelBufs(&cur_body->buf_c_points);
		}	
	}
}