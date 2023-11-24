#include <limbrary/model_view/camera_man.h>
#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera.h>

namespace lim { namespace sdf
{
	void init(CameraController* cam, Light* lit);
    void deinit();
    void bindSdfData(const Program& prog);
    void drawImGui();
	void drawGuizmo(const Viewport& vp);
    void dndCallback(int count, const char **paths);
}}