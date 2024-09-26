#include "app_scene3d.h"
#include <limbrary/tools/log.h>
#include <imgui.h>
#include <limbrary/tools/s_asset_lib.h>
#include <limbrary/tools/limgui.h>

using namespace lim;

AppScene3d::AppScene3d() 
	: AppBase(1400, 780, APP_NAME)
	, viewport(new FramebufferMs())
{
	{
		Model& md = *scene.addOwn(new Model("ground"));
		Material& mat = *md.addOwn(new Material());
		mat.prog = &AssetLib::get().prog_ndv;
		md.root.ms = &AssetLib::get().ground_quad;
		md.root.mat = &mat;
	}
	{
		Model& md = *scene.addOwn(new Model("sphere"));
		Material& mat = *md.addOwn(new Material());
		mat.prog = &AssetLib::get().prog_ndv;
		md.root.ms = &AssetLib::get().sphere;
		md.root.mat = &mat;
	}
	{
		Program& skined_prog = *scene.addOwn(new Program("skinned prog"));
		skined_prog.attatch("mvp_skinned.vs").attatch("ndv.fs").link();

		Model& md = *scene.addOwn(new Model("vam"));
		md.importFromFile("assets/models/jump.fbx", true, true);
		md.setProgToAllMat(&skined_prog);
	}
	dnd_callbacks[this] = [](int count, const char **paths) {
		
	};
}
AppScene3d::~AppScene3d()
{
}
void AppScene3d::update() 
{
	glEnable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	scene.render(viewport.getFb(), viewport.camera);
}
void AppScene3d::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	LimGui::SceneEditor(scene, viewport);
}