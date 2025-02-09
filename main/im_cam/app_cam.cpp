#include "app_cam.h"
#include <limbrary/tools/log.h>
#include <imgui.h>
#include <opencv2/opencv.hpp>

using namespace lim;

AppCam::AppCam() : AppBase(1200, 780, APP_NAME)
	, viewport(new FramebufferNoDepth, "Viewport")
{
	key_callbacks[this] = [](int key, int scancode, int action, int mods) {
		// log::pure("%d\n", key);
		// log::pure("%d\n", key);
		// log::warn("%d\n", key);
		// log::err("%d\n", key);
	};

	cv::utils::logging::setLogLevel(cv::utils::logging::LogLevel::LOG_LEVEL_WARNING);

	cv::Mat image = cv::imread("assets/images/RedFlower-sRGB-yes.jpg"); 
    assert(!image.empty());
    cv::imshow("image", image);
    // cv::waitKey(0); 
}
AppCam::~AppCam()
{
}
void AppCam::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	viewport.getFb().bind();
	viewport.getFb().unbind();
}
void AppCam::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	viewport.drawImGui();
}