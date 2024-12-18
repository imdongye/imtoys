#include "app_skeletal.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <limbrary/3d/mesh_maked.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/limgui.h>
#include <limbrary/tools/asset_lib.h>
#include <functional>

using namespace lim;
using namespace glm;


void AppSkeletal::makeScene(const char* path) {
	ModelData* md;
	scene.reset();

	md = new ModelData();
	md->importFromFile(path, true, true);
	md->setProgToAllMat(&prog_skinned);
	scene.addOwn(md);

	// {
	// 	int nrWidth = 10;
	// 	vec2 posSize{20, 20};
	// 	vec2 startPos{-posSize/vec2{2.f}};
	// 	vec2 stepSize = posSize/vec2{nrWidth-1, nrWidth-1};

	// 	for(int i=0; i<nrWidth; i++) for(int j=0; j<nrWidth; j++)  {
	// 		scene.addOwn(new ModelView(*md));
	// 		vec2 targetPos = startPos+stepSize*vec2{i, j};
	// 		scene.own_mdvs.back()->root.tf.pos.x = targetPos.x;
	// 		scene.own_mdvs.back()->root.tf.pos.z = targetPos.y;
	// 		scene.own_mdvs.back()->root.tf.update();
	// 		scene.own_mdvs.back()->animator.is_loop = true;
	// 		scene.own_mdvs.back()->animator.play();
	// 		scene.own_mdvs.back()->animator.setTimeline(glim::randFloat(), true);
	// 	}
	// }



	// floor
	md = new ModelData();
	md->root.ms = md->addOwn(new MeshPlane(30.f, 30.f));
	md->own_meshes.back()->initGL(true);
	md->root.mat = md->addOwn(new Material());
	md->setProgToAllMat(&prog_static);
	scene.addOwn(md);
}
lim::AppSkeletal::AppSkeletal() : AppBase(1200, 780, APP_NAME, false)
	, viewport(new FramebufferTexDepth())
{
	LightDirectional* lit = new LightDirectional();
	scene.addOwn(lit);

	prog_skinned.name = "skeletal prog";
	prog_skinned.attatch("mvp_skinned.vs").attatch("ndv.fs").link();


	prog_static.name = "static prog";
	prog_static.attatch("mvp.vs").attatch("ndv.fs").link();
	
	makeScene("assets/models/jump.fbx");

	viewport.camera.moveShift(glm::vec3(0,1.4f,0));
	viewport.camera.updateViewMtx();

	dnd_callbacks[this] = [this](int count, const char **paths) {
		makeScene(paths[0]);
	};
}
lim::AppSkeletal::~AppSkeletal()
{
}
static bool drawOffset = true;

void lim::AppSkeletal::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene.render(viewport.getFb(), viewport.camera);


	// viewport.getFb().bind();
	// glDisable(GL_DEPTH_TEST);
	// Program& prog = asset_lib::prog_color;
	// prog.use();
	// viewport.camera.setUniformTo(prog);
	// todo when draw with cloth model getting to screen darker
	//	draw bones 
	// {
	// 	const ModelView& mdview = *scene.own_mdvs[0];
	// 	glm::mat4 globalMtx = mdview.getLocalToBoneRootMtx();
	// 	for( const BoneNode& bone : mdview.animator.bones ) {
	// 		if( drawOffset ) {
	// 			if( bone.idx_weighted_bone<0 )
	// 				return;
	// 			glm::mat4 local = glm::inverse(mdview.src_md->weighted_bone_offsets[bone.idx_weighted_bone]);
	// 			prog.setUniform("mtx_Model", globalMtx * local);
	// 		}
	// 		else {
	// 			prog.setUniform("mtx_Model", globalMtx * bone.mtx_model_space);
	// 		}
	// 		prog.setUniform("color", (LimGui::getPickedBoneNode() == &bone) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1));
	// 		asset_lib::sphere.bindAndDrawGL();
	// 	}
	// }
	// glEnable(GL_DEPTH_TEST);
	// viewport.getFb().unbind();
}
void lim::AppSkeletal::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGuiAndUpdateCam();

	LimGui::ModelViewEditor(*scene.own_mdvs.front());

	ImGui::Begin("skeletal ctrl");
	LimGui::PlotVal("dt", "ms", delta_time);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::Checkbox("draw offset", &drawOffset);
	ImGui::End();
}