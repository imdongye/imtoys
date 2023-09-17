#include <limbrary/model_view/viewport_with_camera.h>
#include <limbrary/app_pref.h>
#include <limbrary/application.h>
#include <imgui.h>

namespace
{
	GLFWwindow* window;
}

namespace lim
{
    // if vp is nullptr then interaction with window
	VpAutoCamera::VpAutoCamera(Viewport *_vp, glm::vec3 _pos, glm::vec3 _focus)
		:Camera(_pos, _focus-_pos)
	{
        vp = _vp;
		pivot = _focus;
		distance = glm::length(position-pivot);
        aspect = vp->width/(float)vp->height;

		// register callbacks
		AppBase& app = *AppPref::get().app;
		window = app.window;
		app.key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
			keyCallback(key, scancode, action, mods);
		};
        vp->resize_callbacks[this] = [this](int w, int h) {
            viewportSizeCallback(w, h);
        };
		app.mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
			mouseBtnCallback(button, action, mods);
		};
		app.cursor_pos_callbacks[this] = [this](double xPos, double yPos) {
			cursorPosCallback(xPos, yPos);
		};
		app.scroll_callbacks[this] = [this](double xOff, double yOff) {
			scrollCallback(xOff, yOff);
		};
		app.update_hooks[this] = [this](float dt) {
			processInput(dt); 
		};
	}
	VpAutoCamera::~VpAutoCamera()
	{
		// unregister callbacks
		AppBase& app = *AppPref::get().app;
		app.key_callbacks.erase(this);
        vp->resize_callbacks.erase(this);
		app.update_hooks.erase(this);
		app.mouse_btn_callbacks.erase(this);
		app.cursor_pos_callbacks.erase(this);
		app.scroll_callbacks.erase(this);
	}
	void VpAutoCamera::setViewMode(int vm)
	{
		viewing_mode = vm%3;
		if( viewing_mode == VM_PIVOT || viewing_mode == VM_SCROLL ) {
			front = pivot-position;
			distance = glm::length(front);
			front = glm::normalize(front);
			// front => yaw pitch
			updateOrientationFromFront();
			// update pivot view mat??
		}
	}
	void VpAutoCamera::copySettingTo(VpAutoCamera& cam)
	{
		cam.position = position;
		cam.orientation = orientation;
		cam.yaw = yaw;
		cam.pitch = pitch;
		cam.roll = roll;
		cam.fovy = fovy;
		cam.distance = distance;
		cam.pivot = pivot;
		cam.front = front;
		cam.up = up;
		cam.right = right;
		cam.view_mat = view_mat;
		cam.proj_mat = proj_mat;
		cam.viewing_mode = viewing_mode;
		cam.move_free_spd = move_free_spd;
		cam.rot_free_spd = rot_free_spd;
		cam.move_pivot_spd = move_pivot_spd;
		cam.rot_pivot_spd = rot_pivot_spd;
		cam.rot_pivot_scroll_spd = rot_pivot_scroll_spd;
		cam.move_pivot_scroll_spd = move_pivot_scroll_spd;
		cam.zoom_spd = zoom_spd;
		cam.zoom_dist_spd = zoom_dist_spd;
	}

	void VpAutoCamera::keyCallback(int key, int scancode, int action, int mods)
	{
		if( vp->hovered && key == GLFW_KEY_TAB && action == GLFW_PRESS ) {
			setViewMode(viewing_mode+1);
		}
	}
	void VpAutoCamera::viewportSizeCallback(int w, int h)
	{
		aspect = w/(float)h;
		updateProjMat();
	}
	void VpAutoCamera::mouseBtnCallback(int button, int action, int mods)
	{
		if( !vp->hovered ) return;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		prev_mouse_x = xpos;
		prev_mouse_y = ypos;
	}
	void VpAutoCamera::cursorPosCallback(double xpos, double ypos)
	{
		if( !vp->dragging ) return;

		float xoff = xpos - prev_mouse_x;
		float yoff = prev_mouse_y - ypos;

		switch( viewing_mode ) {
			case VM_PIVOT:
				if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS 
					|| glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ) {
					shiftPos(xoff * move_pivot_spd, yoff * move_pivot_spd);
					updatePivotViewMat();
				}
				else {
					rotateCamera(xoff * rot_pivot_spd, yoff * rot_pivot_spd);
					updatePivotViewMat();
				}
				break;
			case VM_FREE:
				rotateCamera(xoff * rot_free_spd, yoff * rot_free_spd);
				updateFreeViewMat();
				break;
			case VM_SCROLL:
				break;
		}
		prev_mouse_x = xpos;
		prev_mouse_y = ypos;
	}
	void VpAutoCamera::scrollCallback(double xOff, double yOff)
	{
		if( !vp->hovered || vp->dragging ) return;

		switch( viewing_mode ) {
			case VM_PIVOT:
				shiftDist(yOff * 3.f);
				updatePivotViewMat();
				break;
			case VM_FREE:
				shiftZoom(yOff * 5.f);
				updateProjMat();
				break;
			case VM_SCROLL:
				if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) {
					shiftPos(xOff * move_pivot_scroll_spd, yOff * -move_pivot_scroll_spd);
					updatePivotViewMat();
				}
				else if( glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ) {
					shiftDist(yOff * 3.f);// todo
					updatePivotViewMat();
				}
				else {
					rotateCamera(xOff * rot_pivot_scroll_spd, yOff * -rot_pivot_scroll_spd);
					updatePivotViewMat();
				}
				break;
		}
	}
	void VpAutoCamera::processInput(float dt)
	{
		if( !vp->focused || !vp->hovered )return;

		switch( viewing_mode ) {
			case VM_FREE:
			{
                glm::vec3 dir(0);
				float moveSpd = move_free_spd;
				if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) moveSpd *= 2.f;
				if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ) dir += front;
				if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ) dir -= front;
				if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ) dir -= right;
				if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ) dir += right;
				if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS ) dir += glm::vec3(0, 1, 0);
				if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ) dir -= glm::vec3(0, 1, 0);
				position += moveSpd*dt*dir;

				updateFreeViewMat();
				break;
			}
			case VM_PIVOT:
			case VM_SCROLL:
			{
				updatePivotViewMat();
				break;
			}
		}
		updateProjMat();
	}


    ViewportWithCamera::ViewportWithCamera(std::string_view _name, Framebuffer* createdFB)
        :Viewport(_name, createdFB), camera(this)
    {
    }
	ViewportWithCamera::~ViewportWithCamera()
    {
    }

}