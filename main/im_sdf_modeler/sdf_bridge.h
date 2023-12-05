#ifndef __sdf_bridge_h_
#define __sdf_bridge_h_

#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera_man.h>
#include <filesystem>

namespace lim { namespace sdf
{
	void init(CameraController* cam, Light* lit);
    void deinit();
    void bindSdfData(const Program& prog);
    void drawImGui();
	void drawGuizmo(const Viewport& vp);
    void exportJson(std::filesystem::path path);
    void importJson(std::filesystem::path path);
}}

#endif