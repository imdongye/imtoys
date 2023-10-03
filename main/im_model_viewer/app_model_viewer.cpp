#include "app_model_viewer.h"
#include <imgui.h>
#include <limbrary/model_view/renderer.h>

using namespace lim;
using namespace std;
using namespace glm;


namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(1200, 780, APP_NAME)
	{
		// viewports.reserve(5);
		// models.reserve(5);

		program.name = "model_view";
		program.home_dir = APP_DIR;
		program.attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();

		addModelViewer("assets/models/objs/spot.obj");
	}
	AppModelViewer::~AppModelViewer()
	{
	}

	void AppModelViewer::addModelViewer(const char* path) 
	{
		char* vpName = fmtStrToBuf("viewport%d##model_view", (int)viewports.size());
		viewports.emplace_back(vpName, new RboFramebuffer());
		viewports.back().camera.viewing_mode = VpAutoCamera::VM_PIVOT;
		models.push_back({});
		models.back().importFromFile(path, true);
	}
	void AppModelViewer::drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ ) {
			if( !viewports[i].window_opened ) {
				models.erase(models.begin()+i);
				viewports.erase(viewports.begin()+i);
				i--;
				continue;
			}
			render(viewports[i].getFb(), program, viewports[i].camera, models[i], light);
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
		ImGui::Text("hi");
		ImGui::End();

		for(auto& vp : viewports) {
			vp.drawImGui();
		}
	}
	void AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
	{
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