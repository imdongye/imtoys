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



lim::AppSkeletal::AppSkeletal() : AppBase(1200, 780, APP_NAME)
	, viewport("viewport##skeletal", new FramebufferMs())
{
	LightDirectional* lit = new LightDirectional();
	scene.addOwn(lit);

	program.name = "skeletal prog";
	program.home_dir = APP_DIR;
	program.attatch("skel.vs").attatch("skel.fs").link();

	model.importFromFile("assets/models/jump.fbx", true);
	model.tf->pos.y += model.pivoted_scaled_bottom_height;
	model.tf->update();
	model.setProgToAllMat(&program);
	scene.addRef(&model);
	cur_nod = &model.root;

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
void lim::AppSkeletal::drawHierarchy(lim::RdNode& nd ) {
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
	if( &nd == cur_nod ) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if( nd.childs.size() == 0 ) {
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
	}
	else {
		flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
	}

	if(ImGui::TreeNodeEx(&nd, flags, "%s", nd.name.c_str())) {
		if( cur_nod!=&nd &&ImGui::IsItemClicked(0)) {
			cur_nod = &nd;
		}
		for(auto& c : nd.childs) {
			drawHierarchy(c);
		}
		ImGui::TreePop();
	}	
}
void lim::AppSkeletal::drawInspector(RdNode& nd) {
	ImGui::Text("name : %s", nd.name.c_str());
	ImGui::Text("childs : %d", nd.childs.size());
	bool edited = false;
	edited |= ImGui::DragFloat3("pos", glm::value_ptr(nd.transform.pos), 0.01f);
	glm::vec3 rot = glm::degrees(glm::eulerAngles(nd.transform.ori));
	if(ImGui::DragFloat3("ori", glm::value_ptr(rot), 0.01f)) {
		edited = true;
		nd.transform.ori = glm::quat(glm::radians(rot));
	}
	edited |= ImGui::DragFloat3("scale", glm::value_ptr(nd.transform.scale), 0.01f);
	if( edited ) {
		nd.transform.update();
	}
	LimGui::Mat4(nd.transform.mtx);
}

void lim::AppSkeletal::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	log::drawViewer("logger##template");;

	viewport.drawImGui();

	ImGui::Begin("test");
	if( ImGui::InputInt("BoneIdx", &display_BoneIdx) )
		model.setSetProgToAllMat(makeSetProg());
	ImGui::Text("%d", model.bone_map.size());
	ImGui::End();

	ImGui::Begin("hierarchy");
	drawHierarchy(model.root);
	ImGui::End();

	ImGui::Begin("inspector");
	drawInspector(*cur_nod);
	ImGui::End();

	ImGui::Begin("camera");
	LimGui::Vec3(viewport.camera.position);
	ImGui::End();
}
void lim::AppSkeletal::dndCallback(int cnt, const char **paths) {
	model.importFromFile(paths[0], true);
}