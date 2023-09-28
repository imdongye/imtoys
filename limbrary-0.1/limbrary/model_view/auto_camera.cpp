/*

2023-02-28 / im dongye

윈도우 또는 뷰포트와 3가지 모드로 상호작용하는 카메라

Note:
* Viewport AutoCamera는 Viewport delete하기전에 delete해야함.

Todo:
1. 화면 크기 비례해서 드레그 속도 조절

*/

#include <limbrary/model_view/auto_camera.h>
#include <limbrary/app_pref.h>
#include <limbrary/application.h>
#include <imgui.h>
#include <limbrary/log.h>

namespace
{
	GLFWwindow* _win;
}

namespace lim
{
	AutoCamera::AutoCamera()
	{
	}
	AutoCamera::~AutoCamera()
	{
	}
	void AutoCamera::setViewMode(int vm)
	{
		viewing_mode = vm%3;
	}
	void AutoCamera::keyCallback(int key, int scancode, int action, int mods)
	{
		if( key == GLFW_KEY_TAB && action == GLFW_PRESS ) {
			setViewMode(viewing_mode+1);
		}
	}
	void AutoCamera::viewportSizeCallback(int w, int h)
	{
		aspect = w/(float)h;
		updateProjMat();
	}
	void AutoCamera::mouseBtnCallback(int button, int action, int mods)
	{
		double xpos, ypos;
		glfwGetCursorPos(_win, &xpos, &ypos);
		prev_mouse_x = xpos;
		prev_mouse_y = ypos;
	}
	void AutoCamera::cursorPosCallback(double xpos, double ypos)
	{
		float xoff = xpos - prev_mouse_x;
		float yoff = prev_mouse_y - ypos;

		switch( viewing_mode ) {
			case VM_PIVOT:
				if( glfwGetKey(_win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS 
					|| glfwGetMouseButton(_win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ) {
					shiftPosFromPlane(-xoff * move_pivot_spd, -yoff * move_pivot_spd);
				}
				else {
					rotateCameraFromPivot(xoff * rot_pivot_spd, yoff * rot_pivot_spd);
				}
				break;
			case VM_FREE:
				rotateCamera(xoff * rot_free_spd, yoff * rot_free_spd);
				break;
			case VM_SCROLL:
				break;
		}
		prev_mouse_x = xpos;
		prev_mouse_y = ypos;
	}
	void AutoCamera::scrollCallback(double xOff, double yOff)
	{
		switch( viewing_mode ) {
			case VM_PIVOT:
				shiftDist(yOff * 3.f);
				break;
			case VM_FREE:
				shiftZoom(yOff * 5.f);
				break;
			case VM_SCROLL:
				if( glfwGetKey(_win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) {
					shiftPosFromPlane(-xOff * move_pivot_scroll_spd, yOff * move_pivot_scroll_spd);
				}
				else if( glfwGetKey(_win, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ) {
					shiftDist(yOff * 3.f);// todo
				}
				else {
					rotateCameraFromPivot(xOff * rot_pivot_scroll_spd, yOff * -rot_pivot_scroll_spd);
				}
				break;
		}
	}
	void AutoCamera::processInput(float dt)
	{
		glm::vec3 perallelFront = {front.x, 0.f, front.z};
		perallelFront = glm::normalize(perallelFront);
		switch( viewing_mode ) {
			case VM_FREE:
			{
				glm::vec3 dir(0);
				float moveSpd = move_free_spd;
				if( glfwGetKey(_win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) moveSpd *= 2.f;
				if( glfwGetKey(_win, GLFW_KEY_W) == GLFW_PRESS ) dir += perallelFront;
				if( glfwGetKey(_win, GLFW_KEY_S) == GLFW_PRESS ) dir -= perallelFront;
				if( glfwGetKey(_win, GLFW_KEY_A) == GLFW_PRESS ) dir -= right;
				if( glfwGetKey(_win, GLFW_KEY_D) == GLFW_PRESS ) dir += right;
				if( glfwGetKey(_win, GLFW_KEY_E) == GLFW_PRESS ) dir += glm::vec3(0, 1, 0);
				if( glfwGetKey(_win, GLFW_KEY_Q) == GLFW_PRESS ) dir -= glm::vec3(0, 1, 0);
				shiftPos(moveSpd*dt*dir);
				break;
			}
			case VM_PIVOT:
			case VM_SCROLL:
			{
				break;
			}
		}
	}



	WinAutoCamera::WinAutoCamera()
	{
		AppBase& app = *AppPref::get().app;
		_win = app.window;

		aspect = app.win_width/(float)app.win_height;
		updateProjMat();

		// register callbacks
		app.key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
			keyCallback(key, scancode, action, mods);
		};
		app.framebuffer_size_callbacks[this] = [this](int w, int h) {
			viewportSizeCallback(w, h);
		};
		app.mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			mouseBtnCallback(button, action, mods);
		};
		app.cursor_pos_callbacks[this] = [this](double xPos, double yPos) {
			if( !glfwGetMouseButton(_win, GLFW_MOUSE_BUTTON_LEFT)
			&& !glfwGetMouseButton(_win, GLFW_MOUSE_BUTTON_MIDDLE) )
				return;
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			cursorPosCallback(xPos, yPos);
		};
		app.scroll_callbacks[this] = [this](double xOff, double yOff) {
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			scrollCallback(xOff, yOff);
		};
		app.update_hooks[this] = [this](float dt) {
			processInput(dt); 
		};
	}
	WinAutoCamera::WinAutoCamera(WinAutoCamera&& src) noexcept
		: AutoCamera(std::move(src))
	{
		AppBase& app = *AppPref::get().app;
		app.key_callbacks.changeKey(&src, this);
		app.framebuffer_size_callbacks.changeKey(&src, this);
		app.mouse_btn_callbacks.changeKey(&src, this);
		app.cursor_pos_callbacks.changeKey(&src, this);
		app.scroll_callbacks.changeKey(&src, this);
		app.update_hooks.changeKey(&src, this);
	}
	WinAutoCamera::~WinAutoCamera()
	{
		// unregister callbacks
		AppBase& app = *AppPref::get().app;
		app.framebuffer_size_callbacks.erase(this);
		app.update_hooks.erase(this);
		app.mouse_btn_callbacks.erase(this);
		app.cursor_pos_callbacks.erase(this);
		app.scroll_callbacks.erase(this);
	}

	VpAutoCamera::VpAutoCamera(Viewport* _vp)
	{
        vp = _vp;
		
        aspect = vp->getAspect();
		updateProjMat();

		// register callbacks
		AppBase& app = *AppPref::get().app;
		_win = app.window;
		app.key_callbacks[this] = [this](int key, int scancode, int action, int mods) {
			keyCallback(key, scancode, action, mods);
		};
        vp->resize_callbacks[this] = [this](int w, int h) {
            viewportSizeCallback(w, h);
        };
		app.mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
			if( !vp->hovered )
				return;
			mouseBtnCallback(button, action, mods);
		};
		app.cursor_pos_callbacks[this] = [this](double xPos, double yPos) {
			if( !vp->dragging )
				return;
			cursorPosCallback(xPos, yPos);
		};
		app.scroll_callbacks[this] = [this](double xOff, double yOff) {
			if( !vp->hovered )
				return;
			scrollCallback(xOff, yOff);
		};
		app.update_hooks[this] = [this](float dt) {
			if( !vp->focused )
				return;
			processInput(dt); 
		};
	}
	VpAutoCamera::VpAutoCamera(VpAutoCamera&& src) noexcept
		: AutoCamera(std::move(src))
	{
		vp = src.vp;
		src.vp = nullptr;

		AppBase& app = *AppPref::get().app;
		app.key_callbacks.changeKey(&src, this);
		vp->resize_callbacks.changeKey(&src, this);
		app.mouse_btn_callbacks.changeKey(&src, this);
		app.cursor_pos_callbacks.changeKey(&src, this);
		app.scroll_callbacks.changeKey(&src, this);
		app.update_hooks.changeKey(&src, this);
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
	void VpAutoCamera::copySettingTo(VpAutoCamera& cam)
	{
		cam.position = position;
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
		cam.zoom_fovy_spd = zoom_fovy_spd;
		cam.zoom_dist_spd = zoom_dist_spd;
	}
}