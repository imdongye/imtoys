#include "app_softbody_in_model.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/asset_lib.h>
#include <functional>
#include "pbd/pbd.h"

using namespace lim;
using namespace glm;


namespace {
	float time_speed = 1.f;
	bool is_paused = false;

	int nr_shear = 2;
	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
	float body_mass = 1.f;
	bool is_ptcl_ref_close_verts = true;


	const Mesh* src_mesh = nullptr; // todo recreate soft body
	pbd::SoftBodyGpu* cur_body = nullptr;
}


void AppSoftbodyInModel::reloadModel(const char* path) {
	src_mesh = nullptr;
	LimGui::ModelEditorReset();

	delete scene.own_mds[0];
	Model* srcMd = new Model();
	srcMd->importFromFile(path, true, true);
	srcMd->setProgToAllMat(&prog_skinned);

	scene.own_mds[0] = new ModelView(*srcMd);
}
AppSoftbodyInModel::AppSoftbodyInModel() : AppBase(1480, 780, APP_NAME, false)
	, viewport("viewport##skeletal", new FramebufferTexDepth())
	, ib_light("assets/images/ibls/artist_workshop_4k.hdr")
{

	prog_skinned.name = "skeletal prog";
	prog_skinned.attatch("mvp_skinned_shadow.vs").attatch("im_model_viewer/shaders/brdf.fs").link();

	prog_static.name = "static prog";
	prog_static.attatch("mvp.vs_shadow").attatch("im_model_viewer/shaders/brdf.fs").link();


	scene.ib_light = &ib_light;
	scene.addOwn(new LightDirectional());
	scene.own_lits.back()->setShadowEnabled(true);


	scene.own_mds.resize(2);

	scene.own_mds[0] = nullptr;
	reloadModel("assets/models/jump.fbx");


	// floor
	{
		Model* floorMd = new Model();
		floorMd->name = "floor";
		floorMd->addOwn(new MeshPlane(2.f, 2.f));
		floorMd->own_meshes.back()->initGL(true);
		Material* flMat = floorMd->addOwn(new Material());
		flMat->prog = &prog_static;
		floorMd->root.addMsMat(floorMd->own_meshes.back(), flMat);
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
	viewport.camera.updateViewMat();
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

	render(viewport.getFb(), viewport.camera, scene);
}
void AppSoftbodyInModel::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();


	ModelView& md = *scene.own_mds[0];
	LimGui::ModelEditor(md);
	

	ImGui::Begin("Scene");
	ImGui::Checkbox("draw envMap", &scene.is_draw_env_map);
	ImGui::End();


	ImGui::Begin("pbd ctrl##sbinmd");

	RdNode::MsSet* msset = LimGui::getPickedMsSet();
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("soft body replacer") && msset && msset->transformWhenRender ) {
		ImGui::Text("%s mesh selected", msset->ms->name.c_str());
		ImGui::SliderInt("nr shears", &nr_shear, 0, 2);
		ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4);
		ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);

		if( ImGui::Button("replace") ) {
			cur_body = pbd::replaceMeshInModelToSoftBody(md, *msset
				, nr_shear, bend_type, body_mass, is_ptcl_ref_close_verts);
			phy_scene.bodies.clear();
			phy_scene.bodies.push_back(cur_body);
		}
	}
	else {
		ImGui::Text("not selected");
	}


	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if( ImGui::CollapsingHeader("process") ) {
		ImGui::Checkbox("pause", &is_paused);
		if( ImGui::TreeNode("framerate") ) {
			LimGui::PlotVal("dt", "ms", delta_time*1000.f);
			LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
			ImGui::TreePop();
		}
		ImGui::SliderInt("max fps", &max_fps, 20, 1000);
		ImGui::SliderFloat("time speed", &time_speed, 0.1f, 2.f);
	}


	if( cur_body ) {
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
}
void AppSoftbodyInModel::dndCallback(int _, const char **paths) {
	reloadModel(paths[0]);
}