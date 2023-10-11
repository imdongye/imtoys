#include <limbrary/model_view/camera_man.h>
#include <limbrary/app_pref.h>
#include <limbrary/application.h>
#include <imgui.h>
#include <limbrary/log.h>

namespace lim
{
	CameraCtrlData::CameraCtrlData()
	{
	}
	CameraCtrlData::CameraCtrlData(CameraCtrlData&& src) noexcept
	{
		*this = std::move(src);
	}
	CameraCtrlData& CameraCtrlData::operator=(CameraCtrlData&& src) noexcept
	{
		if( this==&src )
			return *this;
		Camera::operator=(std::move(src));

		viewing_mode = src.viewing_mode;
		move_free_spd = src.move_free_spd;
		rot_free_spd = src.rot_free_spd;
		move_pivot_spd = src.move_pivot_spd;
		rot_pivot_spd = src.rot_pivot_spd;
		move_pivot_scroll_spd = src.move_pivot_scroll_spd;
		rot_pivot_scroll_spd = src.rot_pivot_scroll_spd;
		zoom_fovy_spd = src.zoom_fovy_spd;
		zoom_dist_spd = src.zoom_dist_spd;
		return *this;
	}
	CameraCtrlData::~CameraCtrlData() noexcept
	{
	}
	void CameraCtrlData::setViewMode(int vm)
	{
		viewing_mode = vm%3;
	}



	CameraManWin::CameraManWin()
	{
		initCallbacks();
	}
	CameraManWin::CameraManWin(CameraManWin&& src) noexcept
		: CameraCtrlData(std::move(src))
	{
		src.deinitCallbacks();
		initCallbacks();
	}
	CameraManWin& CameraManWin::operator=(CameraManWin&& src) noexcept
	{
		if( this==&src )
			return *this;
		CameraCtrlData::operator=(std::move(src));

		deinitCallbacks();
		src.deinitCallbacks();

		initCallbacks();
		return *this;
	}
	CameraManWin::~CameraManWin() noexcept
	{
		deinitCallbacks();
	}
	void CameraManWin::deinitCallbacks()
	{
		if(app==nullptr)
			return;
		app->key_callbacks.erase(this);
		app->framebuffer_size_callbacks.erase(this);
		app->cursor_pos_callbacks.erase(this);
		app->scroll_callbacks.erase(this);
		app->update_hooks.erase(this);
	}
	void CameraManWin::initCallbacks()
	{
		if(app)
			deinitCallbacks();

		app = AppPref::get().app;

		aspect = app->win_width/(float)app->win_height;
		updateProjMat();

		// register callbacks
		app->framebuffer_size_callbacks[this] = [this](int w, int h) {
			aspect = w/(float)h;
			updateProjMat();
		};
		app->key_callbacks[this] = [this](int key, int scancode, int action, int mod) {
			if(key==GLFW_KEY_TAB) {
				setViewMode(viewing_mode+1);
			}
		};
		app->cursor_pos_callbacks[this] = [this](double xPos, double yPos) {
			bool isDragging = glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
			isDragging 	   |= glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS;
			isDragging 	   &= !ImGui::GetIO().WantCaptureMouse;
			
			float xoff = xPos - prev_mouse_x;
			float yoff = prev_mouse_y - yPos;
			prev_mouse_x = xPos;
			prev_mouse_y = yPos;

			if(!isDragging)
				return;

			switch( viewing_mode ) {
				case VM_PIVOT:
					if( glfwGetKey(app->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS 
						|| glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS ) {
						shiftOnTangentPlane(-xoff*move_pivot_spd, -yoff*move_pivot_spd);
					}
					else {
						rotateCameraFromPivot(xoff*rot_pivot_spd, yoff*rot_pivot_spd);
					}
					break;
				case VM_FREE:
					rotateCamera(xoff*rot_free_spd, yoff*rot_free_spd);
					break;
				case VM_SCROLL:
					break;
			}
		};
		app->scroll_callbacks[this] = [this](double xOff, double yOff) {
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			switch( viewing_mode ) {
				case VM_PIVOT:
					zoomDist(yOff * 3.f);
					break;
				case VM_FREE:
					zoomFovy(yOff * 5.f);
					updateProjMat();
					break;
				case VM_SCROLL:
					if( glfwGetKey(app->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ) {
						shiftOnTangentPlane(-xOff*move_pivot_scroll_spd, yOff*move_pivot_scroll_spd);
					}
					else if( glfwGetKey(app->window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ) {
						zoomDist(yOff*3.f);// todo
					}
					else {
						rotateCameraFromPivot(xOff*rot_pivot_scroll_spd, -yOff*rot_pivot_scroll_spd);
					}
					break;
			}
		};
		app->update_hooks[this] = [this](float dt) {
			glm::vec3 perallelFront = {front.x, 0.f, front.z};
			perallelFront = glm::normalize(perallelFront);

			switch( viewing_mode ) {
				case VM_FREE: {
					glm::vec3 dir(0);
					float moveSpd = move_free_spd;
					if( glfwGetKey(app->window, GLFW_KEY_LEFT_SHIFT) ) moveSpd *= 2.f;
					if( glfwGetKey(app->window, GLFW_KEY_W) ) dir += perallelFront;
					if( glfwGetKey(app->window, GLFW_KEY_S) ) dir -= perallelFront;
					if( glfwGetKey(app->window, GLFW_KEY_A) ) dir -= right;
					if( glfwGetKey(app->window, GLFW_KEY_D) ) dir += right;
					if( glfwGetKey(app->window, GLFW_KEY_E) ) dir += glm::vec3(0, 1, 0);
					if( glfwGetKey(app->window, GLFW_KEY_Q) ) dir -= glm::vec3(0, 1, 0);
					shift(moveSpd*dt*dir); // Todo: normalize and priority dirction
					break;
				} 
				case VM_PIVOT:
				case VM_SCROLL:
				{
					break;
				}
			}
			// viewmat is always update
			updateViewMat();
		};
	}




	CameraManVp::CameraManVp(Viewport* _vp)
	{
		initCallbacks(_vp);
	}
	CameraManVp::CameraManVp(CameraManVp&& src) noexcept
		:CameraCtrlData(std::move(src))
	{
		src.deinitCallbacks();
		initCallbacks(src.vp);
	}
	CameraManVp& CameraManVp::operator=(CameraManVp&& src) noexcept
	{
		if( this==&src )
			return *this;
		CameraCtrlData::operator=(std::move(src));
		deinitCallbacks();

		src.deinitCallbacks();
		initCallbacks(src.vp);
		return *this;
	}
	CameraManVp::~CameraManVp() noexcept
	{
		deinitCallbacks();
	}
	void CameraManVp::deinitCallbacks()
	{
		if( vp==nullptr )
			return;
        vp->resize_callbacks.erase(this);
		vp->update_callbacks.erase(this);
		vp = nullptr;
	}
	void CameraManVp::initCallbacks(Viewport* _vp)
	{
		if( vp )
			deinitCallbacks();
		vp = _vp;
		if( vp == nullptr )
			return;
		
        aspect = vp->getAspect();
		updateProjMat();
		
        vp->resize_callbacks[this] = [this](int w, int h) {
            aspect = vp->getAspect();
			updateProjMat();
        };
		vp->update_callbacks[this] = [this](float dt) {

			// key callback
			if( ImGui::IsKeyPressed(ImGuiKey_Tab, false) ) {
				setViewMode(viewing_mode+1);
			}

			// cursor pos callback
			if( vp->dragging && (ImGui::IsMouseDown(0)||ImGui::IsMouseDown(2)) ) {
				float xoff = vp->mouse_pos.x - prev_mouse_x;
				float yoff = prev_mouse_y - vp->mouse_pos.y;

				switch( viewing_mode ) {
					case VM_PIVOT:
						if( ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsMouseDown(2) ) {
							shiftOnTangentPlane(-xoff*move_pivot_spd, -yoff*move_pivot_spd);
						}
						else {
							rotateCameraFromPivot(xoff*rot_pivot_spd, yoff*rot_pivot_spd);
						}
						break;
					case VM_FREE:
						rotateCamera(xoff*rot_free_spd, yoff*rot_free_spd);
						break;
					case VM_SCROLL:
						break;
				}
			}
			prev_mouse_x = vp->mouse_pos.x;
			prev_mouse_y = vp->mouse_pos.y;

			// scroll callback
			if( vp->is_mouse_wheeled ) {
				float xOff = vp->mouse_wheel_off.x;
				float yOff = vp->mouse_wheel_off.y;
				switch( viewing_mode ) {
					case VM_PIVOT:
						zoomDist(yOff * 3.f);
						break;
					case VM_FREE:
						zoomFovy(yOff * 5.f);
						updateProjMat();
						break;
					case VM_SCROLL:
						if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
							shiftOnTangentPlane(-xOff*move_pivot_scroll_spd, yOff*move_pivot_scroll_spd);
						}
						else if( ImGui::IsKeyDown(ImGuiKey_LeftAlt) ) {
							zoomDist(yOff*3.f);// todo
						}
						else {
							rotateCameraFromPivot(xOff*rot_pivot_scroll_spd, -yOff*rot_pivot_scroll_spd);
						}
						break;
				}
			}

			// process 
			if( vp->focused ) {
				glm::vec3 perallelFront = {front.x, 0.f, front.z};
				perallelFront = glm::normalize(perallelFront);

				switch( viewing_mode ) {
					case VM_FREE: {
						glm::vec3 dir(0);
						float moveSpd = move_free_spd;
						if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) moveSpd *= 2.f;
						if( ImGui::IsKeyDown(ImGuiKey_W) ) dir += perallelFront;
						if( ImGui::IsKeyDown(ImGuiKey_S) ) dir -= perallelFront;
						if( ImGui::IsKeyDown(ImGuiKey_A) ) dir -= right;
						if( ImGui::IsKeyDown(ImGuiKey_D) ) dir += right;
						if( ImGui::IsKeyDown(ImGuiKey_E) ) dir += glm::vec3(0, 1, 0);
						if( ImGui::IsKeyDown(ImGuiKey_Q) ) dir -= glm::vec3(0, 1, 0);
						shift(moveSpd*dt*dir);
						break;
					}
					case VM_PIVOT:
					case VM_SCROLL:
						break;
				}
				updateViewMat();
			}
		};
	}
	void CameraManVp::copySettingTo(CameraManVp& cam)
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




	ViewportWithCamera::ViewportWithCamera(std::string_view _name, Framebuffer* createdFB)
		: Viewport(_name, createdFB), camera(this)
	{
	}
	ViewportWithCamera::ViewportWithCamera(ViewportWithCamera&& src) noexcept
		: Viewport(std::move(src))
	{
		src.camera.deinitCallbacks();
		camera.operator=(std::move(src.camera));
		camera.initCallbacks(this);
	}
	ViewportWithCamera& ViewportWithCamera::operator=(ViewportWithCamera&& src) noexcept
	{
		if( this==&src )
			return *this;
		camera.deinitCallbacks();
		Viewport::operator=(std::move(src));

		src.camera.deinitCallbacks();
		camera.operator=(std::move(src.camera));
		camera.initCallbacks(this);
		return *this;
	}
	ViewportWithCamera::~ViewportWithCamera() noexcept
	{
	}
}