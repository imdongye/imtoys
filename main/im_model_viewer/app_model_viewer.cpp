#include "app_model_viewer.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <imgui.h>
#include <limbrary/model_view/model.h>
#include <limbrary/log.h>
#include <limbrary/model_view/viewport_with_camera.h>
#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <vector>
#include <limbrary/model_view/renderer.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace 
{
	vector<ViewportWithCamera*> viewports;
	vector<Model*> models;
	Program* program;
	Light* light;

	void addModelViewer(const char* path) {
		char* vpName = fmtStrToBuf("viewport%d##model_view", (int)viewports.size());
		ViewportWithCamera* vp = new ViewportWithCamera(vpName, new RboFramebuffer());
		vp->camera.viewing_mode = VpAutoCamera::VM_PIVOT;
		viewports.push_back(vp);

		Model* md = importModelFromFile(path, true);
		models.push_back(md);
	}

	void drawModelsToViewports()
	{
		for(int i=0; i<viewports.size(); i++ ) {
			if( !viewports[i]->window_opened ) {
				delete models[i];
				delete viewports[i];
				models.erase(models.begin()+i);
				viewports.erase(viewports.begin()+i);
				i--;
				continue;
			}
			const auto& vp = *viewports[i];
			render(*vp.framebuffer,
				   *program, vp.camera, *models[i], *light);
		}
		
	}
}


namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
		program = new Program("model_view", APP_DIR);
		program->attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();
		light = new Light();

		addModelViewer("assets/models/objs/spot.obj");
	}
	AppModelViewer::~AppModelViewer()
	{
		for(auto vp : viewports)
			delete vp;
		viewports.clear();
		for(auto md : models)
			delete md;
		models.clear();
		delete program;
		delete light;
	}
	void AppModelViewer::update() 	
	{
		drawModelsToViewports();

		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void AppModelViewer::renderImGui()
	{
		ImGui::DockSpaceOverViewport();

		log::drawViewer("logger##model_viewer");

		ImGui::Begin("controller##model_viewer");
		ImGui::Text("hi");
		ImGui::End();

		for(ViewportWithCamera* vp : viewports) {
			vp->drawImGui();
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