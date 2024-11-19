/*
    imdongye@naver.com
	fst: 2024-09-23
	lst: 2024-09-23
*/

#ifndef __viewport_with_cam_h_
#define __viewport_with_cam_h_

#include "../viewport.h"
#include "camera_man.h"

namespace lim
{
    class ViewportWithCam : public Viewport
    {
    public:
        CameraManVp camera;
        ViewportWithCam(IFramebuffer* createdFB, const char* _name="Viewport");
        virtual ~ViewportWithCam() noexcept = default;

        inline glm::vec3 getMousePosRayDir() const {
            return camera.screenPosToDir(mouse_uv_pos);
        }
        void movePosFormMouse(glm::vec3& target) const;
        void drawImGuiAndUpdateCam(std::function<void(ViewportWithCam&)> guizmoHook = nullptr); // in viewports.cpp
    };

}

#endif