#include <limbrary/model_view/camera_man.h>
#include <limbrary/program.h>

namespace lim { namespace sdf
{
	void init();
    void deinit();
    void bindSdfData(const Program& prog);
    void drawImGui();
	void drawGuizmo(const Viewport& vp, Camera& cam);
}}