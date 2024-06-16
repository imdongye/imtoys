#include "app_skeletal.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/limgui.h>
#include <functional>

using namespace lim;

static int display_BoneIdx = 0;
static std::function<void(const lim::Program&)> makeSetProg() {
	return [&](const Program& prog) {
		prog.setUniform("display_BoneIdx", display_BoneIdx);
	};
}


void AppSkeletal::importModel(const char* path) {
	model.importFromFile(path, true);
	model.setUnitScaleAndPivot();
	model.tf->pos.y += model.pivoted_scaled_bottom_height;
	model.tf->update();
	model.setProgToAllMat(&program);
	model.setSetProgToAllMat(makeSetProg());
	cur_nd = &model.root;
}
lim::AppSkeletal::AppSkeletal() : AppBase(1200, 780, APP_NAME)
	, viewport("viewport##skeletal", new FramebufferMs())
{
	LightDirectional* lit = new LightDirectional();
	scene.addOwn(lit);

	program.name = "skeletal prog";
	program.home_dir = APP_DIR;
	program.attatch("skel.vs").attatch("skel.fs").link();

	
	importModel("assets/models/jump.fbx");
	scene.addRef(&model);

	// model.animator.play(0);

	Model* floor = new Model();
	Mesh* ms = floor->addOwn(new MeshPlane(1));
	Material* mat = floor->addOwn(new Material());
	floor->root.addMsMat(ms, mat);
	floor->setProgToAllMat(&program);
	scene.addOwn(floor);

	viewport.camera.position.y = model.tf->pos.y;
	viewport.camera.pivot.y = model.tf->pos.y;
	viewport.camera.updateViewMat();

	
}
lim::AppSkeletal::~AppSkeletal()
{
}
void lim::AppSkeletal::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render(viewport.getFb(), viewport.camera, scene);
}
void lim::AppSkeletal::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();

	LimGui::ModelEditor(model);

	ImGui::Begin("camera");
	LimGui::Vec3(viewport.camera.position);
	ImGui::End();
}
void lim::AppSkeletal::dndCallback(int cnt, const char **paths) {
	importModel(paths[0]);
}