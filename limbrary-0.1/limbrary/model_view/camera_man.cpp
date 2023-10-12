#include <limbrary/model_view/camera_man.h>
#include <limbrary/app_pref.h>
#include <limbrary/application.h>
#include <imgui.h>
#include <limbrary/log.h>

using namespace glm;

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
		app->framebuffer_size_callbacks.erase(this);
		app->key_callbacks.erase(this);
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
		app->scroll_callbacks[this] = [this](double xOff, double yOff) {
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			scroll_off = {xOff, yOff};
		};
		app->update_hooks[this] = [this](float dt) {
			is_scrolled = scroll_off.x||scroll_off.y;

			switch( viewing_mode ) {
				case VM_PIVOT:  updatePivotMode(dt); break;
				case VM_FREE:   updateFreeMode(dt); break;
				case VM_SCROLL:	updateScrollMode(dt); break;
			}

			scroll_off = {0,0};
		};
	}
	void CameraManWin::updateFreeMode(float dt)
	{
		const vec3 diff = pivot - position;
		const vec3 front = normalize(diff);
		const vec3 right = normalize(cross(front, global_up));
		const vec3 up    = normalize(cross(right, front));
		const bool isDragging = glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
					    	  | glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS
				 	          | !ImGui::GetIO().WantCaptureMouse;

		// zoom
		if( is_scrolled ) {
			fovy = fovy * pow(1.01f, scroll_off.y);
			fovy = clamp(fovy, MIN_FOVY, MAX_FOVY);
			updateProjMat();
		}

		// move
		const vec3 perallelFront = normalize(vec3{front.x, 0.f, front.z});
		vec3 dir(0);
		float moveSpd = move_free_spd;
		if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) moveSpd *= 2.f;
		if( ImGui::IsKeyDown(ImGuiKey_W) ) dir += perallelFront;
		if( ImGui::IsKeyDown(ImGuiKey_S) ) dir -= perallelFront;
		if( ImGui::IsKeyDown(ImGuiKey_A) ) dir -= right;
		if( ImGui::IsKeyDown(ImGuiKey_D) ) dir += right;
		if( ImGui::IsKeyDown(ImGuiKey_E) ) dir += vec3(0, 1, 0);
		if( ImGui::IsKeyDown(ImGuiKey_Q) ) dir -= vec3(0, 1, 0);
		vec3 step = moveSpd * dt * dir;
		position += step;
		pivot += step;

		// rotate
		if( isDragging && ImGui::IsMouseDown(0) ) {
			vec2 step = rot_free_spd * AppPref::get().app->mouse_off;
			vec3 rotated = rotate(step.y, right) * rotate(-step.x, up)*vec4(diff, 0);
			pivot = rotated + position;
		}

		updateViewMat();
	}
	void CameraManWin::updatePivotMode(float dt)
	{
		const bool isDragging = glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
					    	  | glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS
				 	          | !ImGui::GetIO().WantCaptureMouse;
		// zoom
		if( is_scrolled ) {
			const vec2 step = zoom_dist_spd * scroll_off;
			const vec3 diff = position - pivot;
			float distance = length(diff);
			float newDist = distance * pow(1.01f, step.y);
			newDist = clamp(newDist, MIN_DIST, MAX_DIST);
			position = newDist/distance * diff + pivot;
		}

		if( isDragging ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = AppPref::get().app->mouse_off;

			// move tangent plane
			if( ImGui::IsMouseDown(2) || ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}

			// rotate
			else if( ImGui::IsMouseDown(0) ) {
				const vec2 step = off * rot_pivot_spd;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( is_scrolled||isDragging ) {
			updateViewMat();
		}
	}
	void CameraManWin::updateScrollMode(float dt)
	{
		if( is_scrolled ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = scroll_off;

			// move tangent plane
			if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}
			// zoom
			else if( ImGui::IsKeyDown(ImGuiKey_LeftAlt) ) {
				const vec2 step = zoom_dist_spd * off;
				const vec3 diff = position - pivot;
				float distance = length(diff);
				float newDist = distance * pow(1.01f, step.y);
				newDist = clamp(newDist, MIN_DIST, MAX_DIST);
				position = newDist/distance * diff + pivot;
			}
			// rotate
			else {
				const vec2 step = off * rot_pivot_scroll_spd;
				const vec3 rotated = glm::rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( is_dragging ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = AppPref::get().app->mouse_off;

			// move tangent plane
			if( ImGui::IsMouseDown(2) || ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}

			// rotate
			if( ImGui::IsMouseDown(0) ) {
				const vec2 step = off * rot_pivot_spd;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( vp->is_scrolled||vp->dragging ) {
			updateViewMat();
		}
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
		deinitCallbacks();

		CameraCtrlData::operator=(std::move(src));

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
			if( ImGui::IsKeyPressed(ImGuiKey_Tab, false) ) {
				setViewMode(viewing_mode+1);
			}

			switch( viewing_mode ) {
				case VM_PIVOT:  updatePivotMode(dt); break;
				case VM_FREE:   updateFreeMode(dt); break;
				case VM_SCROLL:	updateScrollMode(dt); break;
			}
		};
	}
	void CameraManVp::copySettingTo(CameraManVp& cam)
	{
		cam.position = position;
		cam.fovy = fovy;
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
	void CameraManVp::updateFreeMode(float dt)
	{
		// zoom
		if( vp->is_scrolled ) {
			fovy = fovy * pow(1.01f, vp->scroll_off.y);
			fovy = clamp(fovy, MIN_FOVY, MAX_FOVY);
			updateProjMat();
		}
		if( vp->focused ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));

			// move
			if( vp->focused ) {
				const vec3 perallelFront = normalize(vec3{front.x, 0.f, front.z});
				vec3 dir(0);
				float moveSpd = move_free_spd;
				if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) moveSpd *= 2.f;
				if( ImGui::IsKeyDown(ImGuiKey_W) ) dir += perallelFront;
				if( ImGui::IsKeyDown(ImGuiKey_S) ) dir -= perallelFront;
				if( ImGui::IsKeyDown(ImGuiKey_A) ) dir -= right;
				if( ImGui::IsKeyDown(ImGuiKey_D) ) dir += right;
				if( ImGui::IsKeyDown(ImGuiKey_E) ) dir += vec3(0, 1, 0);
				if( ImGui::IsKeyDown(ImGuiKey_Q) ) dir -= vec3(0, 1, 0);
				vec3 step = moveSpd * dt * dir;
				position += step;
				pivot += step;
			}

			// rotate
			if( vp->dragging && ImGui::IsMouseDown(0) ) {
				vec2 step = rot_free_spd * AppPref::get().app->mouse_off;
				vec3 rotated = rotate(step.y, right) * rotate(-step.x, up)*vec4(diff, 0);
				pivot = rotated + position;
			}
			updateViewMat();
		}
	}
	void CameraManVp::updatePivotMode(float dt)
	{
		// zoom
		if( vp->is_scrolled ) {
			const vec2 step = zoom_dist_spd * vp->scroll_off;
			const vec3 diff = position - pivot;
			float distance = length(diff);
			float newDist = distance * pow(1.01f, step.y);
			newDist = clamp(newDist, MIN_DIST, MAX_DIST);
			position = newDist/distance * diff + pivot;
		}

		if( vp->dragging ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = AppPref::get().app->mouse_off;

			// move tangent plane
			if( ImGui::IsMouseDown(2) || ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}

			// rotate
			else if( ImGui::IsMouseDown(0) ) {
				const vec2 step = off * rot_pivot_spd;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( vp->is_scrolled||vp->dragging ) {
			updateViewMat();
		}
	}
	void CameraManVp::updateScrollMode(float dt)
	{
		if( vp->is_scrolled ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = vp->scroll_off;

			// move tangent plane
			if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}
			// zoom
			else if( ImGui::IsKeyDown(ImGuiKey_LeftAlt) ) {
				const vec2 step = zoom_dist_spd * off;
				const vec3 diff = position - pivot;
				float distance = length(diff);
				float newDist = distance * pow(1.01f, step.y);
				newDist = clamp(newDist, MIN_DIST, MAX_DIST);
				position = newDist/distance * diff + pivot;
			}
			// rotate
			else {
				const vec2 step = off * rot_pivot_scroll_spd;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( vp->dragging ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = AppPref::get().app->mouse_off;

			// move tangent plane
			if( ImGui::IsMouseDown(2) || ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = move_pivot_spd * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}

			// rotate
			if( ImGui::IsMouseDown(0) ) {
				const vec2 step = off * rot_pivot_spd;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( vp->is_scrolled||vp->dragging ) {
			updateViewMat();
		}
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