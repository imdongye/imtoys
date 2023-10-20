#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
	struct BrdfTestInfo {
		int D = 0;
		int G = 0;
		int F = 0;
	};
	std::vector<BrdfTestInfo> brdfTestInfos;

	function<void(const Program&)> makeSetProg(BrdfTestInfo tInfo) {
		return [=](const Program& prog) {
			prog.setUniform("D", tInfo.D);
			prog.setUniform("G", tInfo.G);
			prog.setUniform("F", tInfo.F);
		};
	}

	void drawModelInfoGUI(Model& md, BrdfTestInfo& tInfo) {
		ImGui::Begin(md.name.c_str());
		ImGui::SliderInt("D", &tInfo.D, 0, 3);
		ImGui::SliderInt("G", &tInfo.G, 0, 3);
		ImGui::SliderInt("F", &tInfo.G, 0, 3);
		ImGui::End();
		md.default_material->set_prog = makeSetProg(tInfo);
	}
}



namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(780, 780, APP_NAME, false)
	{
		viewports.reserve(5);
		scenes.reserve(5);

		program.home_dir = APP_DIR;
		program.attatch("mvp.vs").attatch("debug.fs").link();

		AssetLib::get().default_material.prog = &program;

		addModelViewer("assets/models/objs/spot.obj");
	}
	AppModelViewer::~AppModelViewer()
	{
		brdfTestInfos.clear();
	}

	void AppModelViewer::addModelViewer(string path) 
	{
		Model* md = new Model();
		if( md->importFromFile(findModelInDirectory(path), true)==false )
			return;

		md->my_materials.push_back(new Material());
		md->default_material = md->my_materials.back();
		md->default_material->prog = &program;

		brdfTestInfos.push_back({});
		md->default_material->set_prog = makeSetProg(brdfTestInfos.back());


		Scene scn;
		scn.lights.push_back(&light);
		scn.models.push_back(md);
		scn.my_mds.push_back(md); // 0번째
		scenes.emplace_back(std::move(scn));

		char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
		viewports.emplace_back(vpName, new FramebufferMs(8));
		viewports.back().camera.setViewMode(CameraManVp::VM_PIVOT);
	}
	void AppModelViewer::subModelViewer(int idx)
	{
		scenes.erase(scenes.begin()+idx);
		viewports.erase(viewports.begin()+idx);
		brdfTestInfos.erase(brdfTestInfos.begin()+idx);
	}
	
	void AppModelViewer::drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ ) {
			render(viewports[i].getFb(), viewports[i].camera, scenes[i]);
			if( !viewports[i].is_opened ) {
				subModelViewer(i);
				i--;
				continue;
			}
		}
	}


	void AppModelViewer::update() 	
	{
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawModelsToViewports();
	}
	void AppModelViewer::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		log::drawViewer("logger##model_viewer");

		for(int i=0; i<scenes.size(); i++) {
			drawModelInfoGUI(*scenes[i].my_mds[0], brdfTestInfos[i]);
			viewports[i].drawImGui();
		}
	}
	void AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
	{
		// glFinish후 호출돼서 여기서 프로그램 리로드 해도됨.
		if( ( GLFW_MOD_CONTROL == mods ) && ( 'R' == key ) ) {
			program.reload(GL_FRAGMENT_SHADER);
		}
	}
	void AppModelViewer::cursorPosCallback(double xPos, double yPos)
	{
	}
	void AppModelViewer::dndCallback(int count, const char **paths)
	{
		for( int i=0; i<count; i++ ) {
			const char* path = paths[i];
			addModelViewer(path);
		}
	}
}