#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace
{
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
	}

	void AppModelViewer::addModelViewer(string path) 
	{
		Model* md = new Model();
		if( md->importFromFile(findModelInDirectory(path), true)==false )
			return;
		Model* refMd = new Model(*md, true);
		refMd->position += vec3(1,0,0);
		refMd->updateModelMat();

		Scene scn;
		scn.addLight(&light);
		scn.addModel(md, true);
		scn.addModel(refMd, true);
		scenes.emplace_back(std::move(scn));

		char* vpName = fmtStrToBuf("%s##model_view", md->name.c_str());
		viewports.emplace_back(vpName, new FramebufferMs(8));
		viewports.back().camera.viewing_mode = VpAutoCamera::VM_PIVOT;
	}
	
	void AppModelViewer::drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ ) {
			if( !viewports[i].window_opened ) {
				scenes.erase(scenes.begin()+i);
				viewports.erase(viewports.begin()+i);
				i--;
				continue;
			}
			render(viewports[i].getFb(), viewports[i].camera, scenes[i]);
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

		ImGui::Begin("controller##model_viewer");
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
        
		if(ImGui::Button("reload .fs")) {
			program.reload(GL_FRAGMENT_SHADER);
		}
		ImGui::End();

		for(auto& vp : viewports) {
			vp.drawImGui();
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