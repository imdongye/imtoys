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
void lim::AppSkeletal::drawInspector(RdNode& nd) {
	ImGui::Text("name : %s", nd.name.c_str());
	ImGui::Text("childs : %d", nd.childs.size());
	bool edited = false;
	edited |= ImGui::DragFloat3("pos", glm::value_ptr(nd.tf.pos), 0.01f);
	edited |= ImGui::DragFloat3("scale", glm::value_ptr(nd.tf.scale), 0.01f);
	glm::vec3 rot = glm::degrees(glm::eulerAngles(nd.tf.ori));
	if(ImGui::DragFloat3("ori", glm::value_ptr(rot), 0.1f)) {
		edited = true;
		nd.tf.ori = glm::quat(glm::radians(rot));
	}
	if( edited ) {
		nd.tf.update();
	}
	LimGui::Mat4(nd.tf.mtx);
}

void lim::AppSkeletal::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();

	ImGui::Begin("bone info");
	ImGui::Text("nr_bones : %d", model.animator.nr_bones);
	ImGui::Text("nr_animations : %d", model.animator.animations.size());
	switch(model.animator.state) {
	case Animator::State::PLAY: ImGui::Text("state : PLAY"); break;
	case Animator::State::PAUSE: ImGui::Text("state : PAUSE"); break;
	case Animator::State::STOP: ImGui::Text("state : STOP"); break;
	}
	float progress = model.animator.elapsed_sec / model.animator.duration_sec; 
	ImGui::ProgressBar(progress);


	if(ImGui::Button("Play")) {
		model.animator.play(0);
	}
	ImGui::SameLine();
	if(ImGui::Button("Pause")) {
		model.animator.pause();
	}
	ImGui::SameLine();
	if(ImGui::Button("Stop")) {
		model.animator.stop();
	}
	ImGui::SameLine();
	ImGui::Checkbox("isLoop", &model.animator.is_loop);

	if( ImGui::InputInt("display bone Idx", &display_BoneIdx) ) {
		model.setSetProgToAllMat(makeSetProg());
	}

	// if(ImGui::Button("default")) {
	// 	model.animator.updateDefaultMtxBones();
	// }
	std::vector<const char*> anim_list;
	for(auto& anim : model.animator.animations) {
		anim_list.push_back(anim.name.c_str());
	}
	static int selected_anim = 0;
	if(ImGui::Combo("animation", &selected_anim, anim_list.data(), anim_list.size())) {
		model.animator.play(selected_anim);
	}
	ImGui::End();

	ImGui::Begin("camera");
	LimGui::Vec3(viewport.camera.position);
	ImGui::End();
}
void lim::AppSkeletal::dndCallback(int cnt, const char **paths) {
	importModel(paths[0]);
}