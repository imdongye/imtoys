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

namespace
{
	GLFWwindow* window;
}

namespace lim
{
	// if vp is nullptr then interaction with window
	AutoCamera::AutoCamera(glm::vec3 _pos, glm::vec3 _focus)
		:Camera(_pos, _focus-_pos)
	{
		pivot = _focus;
		distance = glm::length(position-pivot);

		AppBase& app = *AppPref::get().app;
		window = app.window;

		aspect = app.win_width/(float)app.win_height;

		// register callbacks
		app.framebuffer_size_callbacks[this] = [this](int w, int h) {
			viewportSizeCallback(w, h);
		};
		app.update_hooks[this] = [this](float dt) {
			processInput(dt); 
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
	}
	AutoCamera::~AutoCamera()
	{
		// unregister callbacks
		AppBase& app = *AppPref::get().app;
		app.framebuffer_size_callbacks.erase(this);
		app.update_hooks.erase(this);
		app.mouse_btn_callbacks.erase(this);
		app.cursor_pos_callbacks.erase(this);
		app.scroll_callbacks.erase(this);
	}
	void AutoCamera::setViewMode(int vm)
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
		if( ImGui::GetIO().WantCaptureMouse ) return;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		prev_mouse_x = xpos;
		prev_mouse_y = ypos;
	}
	void AutoCamera::cursorPosCallback(double xpos, double ypos)
	{
		if( !glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)
			&& !glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) )
			return;
		if( ImGui::GetIO().WantCaptureMouse )
			return;


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
	void AutoCamera::scrollCallback(double xOff, double yOff)
	{
		if( ImGui::GetIO().WantCaptureMouse ) return;

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
	void AutoCamera::processInput(float dt)
	{
		switch( viewing_mode ) {
			case VM_FREE:
			{
				glm::vec3 dir(0);
				float moveSpd =move_free_spd;
				if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) moveSpd = move_free_spd_fast;
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
}