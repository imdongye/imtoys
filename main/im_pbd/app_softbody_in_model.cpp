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
	bool is_pause = false;

	int nr_shear = 2;
	pbd::SoftBody::BendType bend_type = pbd::SoftBody::BT_DISTANCE;
	float body_mass = 1.f;
	bool is_ptcl_ref_close_verts = true;
}


void AppSoftbodyInModel::makeScene(const char* path) {
	scene.clear();
	LimGui::ModelEditorReset();

	delete src_model;
	src_model = new Model();
	src_model->importFromFile(path, true, true);
	src_model->setProgToAllMat(&prog_skinned);

	scene.addOwn(new ModelView(*src_model));


	// {
	// 	int nrWidth = 10;
	// 	vec2 posSize{20, 20};
	// 	vec2 startPos{-posSize/vec2{2.f}};
	// 	vec2 stepSize = posSize/vec2{nrWidth-1, nrWidth-1};

	// 	for(int i=0; i<nrWidth; i++) for(int j=0; j<nrWidth; j++)  {
	// 		scene.addOwn(new ModelView(*md));
	// 		vec2 targetPos = startPos+stepSize*vec2{i, j};
	// 		scene.own_mds.back()->root.tf.pos.x = targetPos.x;
	// 		scene.own_mds.back()->root.tf.pos.z = targetPos.y;
	// 		scene.own_mds.back()->root.tf.update();
	// 		scene.own_mds.back()->animator.is_loop = true;
	// 		scene.own_mds.back()->animator.play();
	// 		scene.own_mds.back()->animator.setTimeline(glim::randFloat(), true);
	// 	}
	// }



	// floor
	Model* floor = new Model();
	Mesh* floorMs = floor->addOwn(new MeshPlane(30.f, 30.f));
	floorMs->initGL(true);
	Material* mat = floor->addOwn(new Material());
	floor->root.addMsMat(floorMs, mat);
	floor->setProgToAllMat(&prog_static);
	scene.addOwn(floor);
}
AppSoftbodyInModel::AppSoftbodyInModel() : AppBase(1200, 780, APP_NAME, false)
	, viewport("viewport##skeletal", new FramebufferTexDepth())
{
	LightDirectional* lit = new LightDirectional();
	scene.addOwn(lit);

	prog_skinned.name = "skeletal prog";
	prog_skinned.attatch("mvp_skinned.vs").attatch("ndv.fs").link();


	prog_static.name = "static prog";
	prog_static.attatch("mvp.vs").attatch("ndv.fs").link();
	
	makeScene("assets/models/jump.fbx");

	viewport.camera.moveShift(glm::vec3(0,1.4f,0));
	viewport.camera.updateViewMat();
}
AppSoftbodyInModel::~AppSoftbodyInModel()
{
	delete src_model;
}
static bool drawOffset = true;

void AppSoftbodyInModel::update() 
{
	if( !is_pause ) {
		phy_scene.update(delta_time);
	}


	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render(viewport.getFb(), viewport.camera, scene);


	const IFramebuffer& fb = viewport.getFb();
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	glViewport(0, 0, fb.width, fb.height);
	glDisable(GL_DEPTH_TEST);
	Program& prog = AssetLib::get().prog_color;
	prog.use();
	viewport.camera.setUniformTo(prog);


	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void AppSoftbodyInModel::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();


	ModelView& md = *scene.own_mds.front();
	LimGui::ModelEditor(md);
	



	ImGui::Begin("app ctrl##sbinmd");
	{
		RdNode::MsSet* msset = LimGui::getPickedMsSet();
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if( msset && ImGui::CollapsingHeader("soft body replacer") ) {
			ImGui::Text("%s mesh selected", msset->ms->name.c_str());
			ImGui::SliderInt("nr shears", &nr_shear, 0, 2);
			ImGui::Combo("bendType", (int*)&bend_type, "none\0distance\0dihedral\0isometric\0", 4);
			ImGui::Checkbox("ptcl ref close verts", &is_ptcl_ref_close_verts);

			if( ImGui::Button("replace") ) {
				pbd::SoftBodyGpu* sb = pbd::replaceMeshInModelToSoftBody(md, *msset
					, nr_shear, bend_type, body_mass, is_ptcl_ref_close_verts);
				phy_scene.bodies.clear();
				phy_scene.bodies.push_back(sb);
			}
		}
		else {
			ImGui::Text("not selected");
		}


		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if( ImGui::CollapsingHeader("Simulate") ) {
			ImGui::Checkbox("pause", &is_pause);
		}


		if( ImGui::CollapsingHeader("Debug") ) {
			LimGui::PlotVal("dt", "ms", delta_time);
			LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
			ImGui::Checkbox("draw offset", &drawOffset);
		}
	}
	ImGui::End();
}
void AppSoftbodyInModel::dndCallback(int _, const char **paths) {
	makeScene(paths[0]);
}