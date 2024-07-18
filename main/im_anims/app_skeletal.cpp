#include "app_skeletal.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/limgui.h>
#include <functional>
#include <limbrary/asset_lib.h>
#include <glm/gtc/random.hpp>

using namespace lim;
using namespace glm;


void AppSkeletal::importModel(const char* path) {
	model.importFromFile(path, true);
	model.setUnitScaleAndPivot();
	model.tf->pos.y += model.pivoted_scaled_bottom_height;
	model.tf->update();
	model.setProgToAllMat(&program);
	LimGui::ModelEditorReset();
}
lim::AppSkeletal::AppSkeletal() : AppBase(1200, 780, APP_NAME, false)
	, viewport("viewport##skeletal", new FramebufferTexDepth())
{
	LightDirectional* lit = new LightDirectional();
	scene.addOwn(lit);

	program.name = "skeletal prog";
	program.attatch("mvp_skinned.vs").attatch("ndv.fs").link();

	
	importModel("assets/models/jump.fbx");
	scene.addRef(&model);


	// {
	// 	int nrWidth = 10;
	// 	vec2 posSize{10, 10};
	// 	vec2 startPos{-posSize/vec2{2.f}};
	// 	vec2 stepSize = posSize/vec2{nrWidth-1, nrWidth-1};

	// 	for(int i=0; i<nrWidth; i++) for(int j=0; j<nrWidth; j++)  {
	// 		scene.addOwn(new ModelView(model));
	// 		vec2 targetPos = startPos+stepSize*vec2{i, j};
	// 		scene.own_mds.back()->tf->pos.x = targetPos.x;
	// 		scene.own_mds.back()->tf->pos.z = targetPos.y;
	// 		scene.own_mds.back()->tf->update();
	// 		scene.own_mds.back()->animator.is_loop = true;
	// 		scene.own_mds.back()->animator.play();
	// 		// scene.own_mds.back()->animator.setTimeline(randFloat(), true);
	// 	}
	// }



	Model* floor = new Model();
	Mesh* ms = floor->addOwn(new MeshPlane(10.f));
	Material* mat = floor->addOwn(new Material());
	floor->root.addMsMat(ms, mat);
	floor->setProgToAllMat(&program);
	scene.addOwn(floor);

	viewport.camera.moveShift(glm::vec3(0,model.tf->pos.y,0));
	viewport.camera.updateViewMat();

	
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

	render(viewport.getFb(), viewport.camera, scene);


	const IFramebuffer& fb = viewport.getFb();
	glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);
	glViewport(0, 0, fb.width, fb.height);
	glDisable(GL_DEPTH_TEST);
	Program& prog = AssetLib::get().color_prog;
	prog.use();
	viewport.camera.setUniformTo(prog);
	glm::mat4 globalMtx = model.getGlobalTfMtx();
	for( const BoneNode& bone : model.animator.skeleton ) {
		if( drawOffset ) {
			if( bone.idx_bone<0 )
				return;
			glm::mat4 local = glm::inverse(model.bone_offsets[bone.idx_bone]);
			prog.setUniform("mtx_Model", globalMtx * local);
		}
		else {
			prog.setUniform("mtx_Model", globalMtx * bone.tf_model_space);
		}
		prog.setUniform("color", (LimGui::getPickedBoneNode() == &bone) ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1));
		AssetLib::get().sphere.bindAndDrawGL();
	}
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void lim::AppSkeletal::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();

	LimGui::ModelEditor(model);
	// LimGui::ModelEditor(*scene.own_mds.front());

	ImGui::Begin("skeletal ctrl");
	LimGui::PlotVal("dt", "ms", delta_time);
	LimGui::PlotVal("fps", "", ImGui::GetIO().Framerate);
	ImGui::Checkbox("draw offset", &drawOffset);
	ImGui::End();
}
void lim::AppSkeletal::dndCallback(int cnt, const char **paths) {
	importModel(paths[0]);
}