#include <limbrary/model_view/camera_man.h>
#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera.h>

namespace lim { namespace sdf
{
	void init(Camera* cam, Light* lit);
    void deinit();
    void bindSdfData(const Program& prog);
    void drawImGui();
	void drawGuizmo(const Viewport& vp);
}}