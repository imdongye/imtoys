//
//	윈도우 또는 뷰포트와 3가지 모드로 상호작용하는 카메라
// 
//  2023-02-28 / im dongye
// 
//	Todo:
//	1. 화면 크기 비례해서 드레그 속도 조절
//

#ifndef AUTO_CAMERA_H
#define AUTO_CAMERA_H

namespace lim
{
	class AutoCamera : public Camera
	{
	public:
		enum VIEWING_MODE
		{
			VM_FREE=0,
			VM_PIVOT,
			VM_SCROLL
		};
		int viewing_mode;
		GLFWwindow *window;
		Viewport *vp;
	private:
		float move_free_spd = 4.2;
		float move_free_spd_fast = 8.2;
		float rot_free_spd = 0.09;

		float move_pivot_spd = -0.003;
		float rot_pivot_spd = 0.003;

		float rot_pivot_scroll_spd = 0.045;
		float move_pivot_scroll_spd = -0.09;

		float zoom_spd = 1;
		float zoom_dist_spd = 1;

		double cur_x_off = 0;
		double cur_y_off = 0;
		AutoCamera(const AutoCamera&)=delete;
		AutoCamera& operator=(const AutoCamera&) = delete;
	public:
		// if vp is nullptr then interaction with window
		AutoCamera(GLFWwindow* _win, Viewport *_vp = nullptr
				   , int viewingMode = VM_FREE
				   , glm::vec3 _pos = {0,0,8}, glm::vec3 _focus = {0,0,0})
			:Camera(_pos, _focus-_pos), window(_win), vp(_vp), viewing_mode(viewingMode)
		{
			pivot = _focus;
			distance = glm::length(position-pivot);

			// register callbacks
			AppBase::WindowData &data = *(AppBase::WindowData*)glfwGetWindowUserPointer(window);
			if( !vp ) {
				int w, h;
				glfwGetFramebufferSize(window, &w, &h);
				aspect = w/(float)h;
				data.framebuffer_size_callbacks[this] = [this](int w, int h) {
					viewportSizeCallback(w, h);
				};
			}
			else {
				aspect = vp->width/(float)vp->height;
				vp->resize_callbacks[this] = [this](int w, int h) {
					viewportSizeCallback(w, h);
				};
			}
			data.update_hooks[this] = [this](float dt) {
				processInput(dt); 
			};
			data.mouse_btn_callbacks[this] = [this](int button, int action, int mods) {
				mouseBtnCallback(button, action, mods);
			};
			data.cursor_pos_callbacks[this] = [this](double xPos, double yPos) {
				cursorPosCallback(xPos, yPos);
			};
			data.scroll_callbacks[this] = [this](double xOff, double yOff) {
				scrollCallback(xOff, yOff);
			};
		}
		virtual ~AutoCamera()
		{
			// unregister callbacks
			AppBase::WindowData &data = *(AppBase::WindowData*)glfwGetWindowUserPointer(window);
			if( !vp ) {
				data.framebuffer_size_callbacks.erase(this);
			}
			else {
				vp->resize_callbacks.erase(this);
			}
			data.update_hooks.erase(this);
			data.mouse_btn_callbacks.erase(this);
			data.cursor_pos_callbacks.erase(this);
			data.scroll_callbacks.erase(this);
		}
	public:
		void setViewMode(int vm)
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
	private:
		void keyCallback(int key, int scancode, int action, int mods)
		{
			if( key == GLFW_KEY_TAB && action == GLFW_PRESS ) {
				setViewMode(viewing_mode+1);
			}
		}
		void viewportSizeCallback(int w, int h)
		{
			aspect = w/(float)h;
		}
		void mouseBtnCallback(int button, int action, int mods)
		{
			if( vp && !vp->hovered ) return;

			glfwGetCursorPos(window, &cur_x_off, &cur_y_off);
		}
		void cursorPosCallback(double xpos, double ypos)
		{
			static float xoff, yoff;

			if( vp && !vp->dragging )return;

			xoff = xpos - cur_x_off;
			yoff = cur_y_off - ypos;

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
			cur_x_off = xpos;
			cur_y_off = ypos;
		}
		void scrollCallback(double xOff, double yOff)
		{
			if( vp && !vp->hovered )return;

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
		void processInput(float dt)
		{
			glm::vec3 dir(0);
			if( vp && !vp->focused )return;

			switch( viewing_mode ) {
				case VM_FREE:
				{
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
	};
}

#endif