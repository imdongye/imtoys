#include "app_scene3d.h"
#include <limbrary/tools/log.h>
#include <imgui.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/limgui.h>

using namespace lim;

AppScene3d::AppScene3d() 
	: AppBase(1400, 780, APP_NAME)
	, vp(new FramebufferMs())
{
	{
		Program& prog = *scene.addOwn(new Program("pcss prog"));
		prog.attatch("mvp_shadow.vs").attatch("pcss.fs").link();

		ModelData& md = *scene.addOwn(new ModelData("ground"));
		Material& mat = *md.addOwn(new Material());
		mat.prog = scene.own_progs[0].raw;
		md.root.ms = asset_lib::big_plane;
		md.root.mat = &mat;
	}
	{
		ModelData& md = *scene.addOwn(new ModelData("cylinder"));
		Material& mat = *md.addOwn(new Material());
		mat.prog = scene.own_progs[0].raw;
		md.root.ms = asset_lib::thin_cylinder;
		md.root.tf.pos.x = -1.f;
		md.root.tf.update();
		md.root.mat = &mat;
	}
	{
		Program& skined_prog = *scene.addOwn(new Program("skinned prog"));
		skined_prog.attatch("mvp_skinned_shadow.vs").attatch("pcss.fs").link();

		ModelData& md = *scene.addOwn(new ModelData("vam"));
		md.importFromFile("assets/models/jump.fbx", true, true);
		md.setProgToAllMat(&skined_prog);
	}
	{
		LightDirectional& lit = *scene.addOwn(new LightDirectional(true));
	}

	vp.camera.pos.y = 1.f;
	vp.camera.updateViewMtx();

	// dnd_callbacks[this] = [](int count, const char **paths) {
		
	// };
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

	scene.render(vp.getFb(), vp.camera, true);
}
void AppScene3d::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	LimGui::SceneEditor(scene, vp);
}