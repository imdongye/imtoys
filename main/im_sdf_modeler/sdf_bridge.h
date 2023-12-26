#ifndef __sdf_bridge_h_
#define __sdf_bridge_h_

#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera_man.h>
#include <filesystem>

namespace sdf
{
	void init(lim::CameraController* cam, lim::Light* lit);
    void deinit();
    void bindSdfData(const lim::Program& prog);
    void drawImGui();
	void drawGuizmo(const lim::Viewport& vp);
    void exportJson(std::filesystem::path path);
    void importJson(std::filesystem::path path);
    void clickCallback(int btn, glm::vec2 uv);
    void keyCallback(int key, int scancode, int action, int mods);
}

#endif