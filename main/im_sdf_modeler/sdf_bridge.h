#ifndef __sdf_bridge_h_
#define __sdf_bridge_h_

#include <limbrary/program.h>
#include <limbrary/3d/light.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <filesystem>

namespace sdf
{
	void init(lim::CameraCtrl* cam);
    void deinit();
    void bindSdfData(const lim::Program& prog);
    void drawImGui();
	void drawGuizmo(lim::ViewportWithCam& vp);
    void exportJson(std::filesystem::path path);
    void importJson(std::filesystem::path path);
    void clickCallback(int btn, glm::vec2 uv);
    void keyCallback(int key, int scancode, int action, int mods);
}

#endif