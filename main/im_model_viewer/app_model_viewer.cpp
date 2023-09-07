#include "app_model_viewer.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <limbrary/log.h>
#include <imgui.h>
#include <limbrary/model_view/viewport_with_camera.h>

using namespace lim;
using namespace std;
using namespace glm;

namespace 
{
	vector<ViewportWithCamera*> viewports;
	void addViewport() {
		ViewportWithCamera* vp = new ViewportWithCamera(new RboFramebuffer());

		viewports.push_back(vp);
	}
}


namespace lim
{
	AppModelViewer::AppModelViewer() : AppBase(1200, 780, APP_NAME)
	{
		stbi_set_flip_vertically_on_load(true);
		log::err("asdfasdf");
	}
	AppModelViewer::~AppModelViewer()
	{
		for(auto vp : viewports)
			delete vp;
		viewports.clear();
		
	}
	void AppModelViewer::update() 	{
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
		if(ImGui::Button("add viewport")) {
			addViewport();
		}
		ImGui::End();

		for(ViewportWithCamera* vp : viewports) {
			vp->drawImGui();
		}
	}
	void AppModelViewer::keyCallback(int key, int scancode, int action, int mods)
	{
		log::pure("%d\n", key);
		log::info("%d\n", key);
		log::warn("%d\n", key);
		log::err("%d\n", key);
	}
	void AppModelViewer::cursorPosCallback(double xPos, double yPos)
	{
	}
}