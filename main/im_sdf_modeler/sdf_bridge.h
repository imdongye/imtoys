#ifndef __sdf_bridge_h_
#define __sdf_bridge_h_

#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera_man.h>
#include <filesystem>

namespace sdf
{
    void clear();
	void init(lim::CameraController* cam, lim::Light* lit);
    void bindSdfData(const lim::Program& prog);
    void drawImGui();
	void drawGuizmo(const lim::Viewport& vp);
    void exportJson(std::filesystem::path path);
    void importJson(std::filesystem::path path);
}

#endif