#include <limbrary/model_view/camera_man.h>
#include <limbrary/app_pref.h>
#include <limbrary/application.h>
#include <imgui.h>
#include <limbrary/log.h>
#include <glm/gtx/transform.hpp>

using namespace glm;

namespace lim
{
	CameraController::CameraController()
	{
	}
	CameraController::CameraController(CameraController&& src) noexcept
	{
		*this = std::move(src);
	}
	CameraController& CameraController::operator=(CameraController&& src) noexcept
	{
		if( this==&src )
			return *this;
		Camera::operator=(std::move(src));

		viewing_mode = src.viewing_mode;
		spd_free_move = src.spd_free_move;
		spd_free_rot = src.spd_free_rot;
		spd_pivot_move = src.spd_pivot_move;
		spd_pivot_rot = src.spd_pivot_rot;
		spd_scroll_move = src.spd_scroll_move;
		spd_scroll_rot = src.spd_scroll_rot;
		spd_zoom_fovy = src.spd_zoom_fovy;
		spd_zoom_dist = src.spd_zoom_dist;
		return *this;
	}
	CameraController::~CameraController() noexcept
	{
	}
	void CameraController::setViewMode(int vm)
	{
		viewing_mode = vm%3;
	}
	void CameraController::updateFreeMode()
	{
		// zoom
		if( input_status&IST_SCROLLED ) {
			fovy = fovy * pow(1.01f, scroll_off.y);
			fovy = clamp(fovy, MIN_FOVY, MAX_FOVY);
			updateProjMat();
		}
		if( input_status&IST_FOCUSED ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));

			// move
			const vec3 forwardDir = (1)?front:normalize(vec3{front.x, 0.f, front.z});
			vec3 dir(0);
			float moveSpd = spd_free_move;
			if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) { moveSpd *= 2.f; }
			if( ImGui::IsKeyDown(ImGuiKey_W) ) { dir += forwardDir; }
			if( ImGui::IsKeyDown(ImGuiKey_S) ) { dir -= forwardDir; }
			if( ImGui::IsKeyDown(ImGuiKey_E) ) { dir += global_up; }
			if( ImGui::IsKeyDown(ImGuiKey_Q) ) { dir -= global_up; }
			if( ImGui::IsKeyDown(ImGuiKey_A) ) {
				if( !ImGui::IsKeyDown(ImGuiKey_D) ) {
				 	dir -= right; is_left = true;
				}
				else if( !prev_is_left ) {
				 	dir -= right; is_left = false;
				}
			}
			if( ImGui::IsKeyDown(ImGuiKey_D) ) {
				if( !ImGui::IsKeyDown(ImGuiKey_A) ) {
				 	dir += right; is_left = false;
				}
				else if( prev_is_left ) {
				 	dir += right; is_left = true;
				}
			}
			if( dir!=vec3(0) ) {
				moveShift(moveSpd*ImGui::GetIO().DeltaTime*normalize(dir));
				prev_is_left = is_left; 
			}

			// rotate
			if( input_status&IST_DRAGGED && ImGui::IsMouseDown(0) ) {
				vec2 step = spd_free_rot * mouse_off;
				vec3 rotated = rotate(-step.x, up)*rotate(-step.y, right)*vec4(diff, 0);
				pivot = rotated + position;
			}
			updateViewMat();
		}
	}
	void CameraController::updatePivotMode()
	{
		// zoom
		if( input_status&IST_SCROLLED ) {
			const vec2 step = spd_zoom_dist * scroll_off;
			const vec3 diff = position - pivot;
			float distance = length(diff);
			float newDist = distance * pow(1.01f, step.y);
			newDist = clamp(newDist, MIN_DIST, MAX_DIST);
			position = newDist/distance * diff + pivot;
		}

		if( input_status&IST_DRAGGED ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = mouse_off;

			// move tangent plane
			if( ImGui::IsMouseDown(2) || ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = spd_pivot_move * vec3(up*off.y -right*off.x);
				moveShift(step);
			}

			// rotate
			else if( ImGui::IsMouseDown(0) ) {
				const vec2 step = off * spd_pivot_rot;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.99f )
					position = pivot + rotated;
			}
		}

		if( input_status&(IST_SCROLLED|IST_DRAGGED) ) {
			updateViewMat();
		}
	}
	void CameraController::updateScrollMode()
	{
		if( input_status&IST_SCROLLED ) {
			const vec3 diff = pivot - position;
			const vec3 front = normalize(diff);
			const vec3 right = normalize(cross(front, global_up));
			const vec3 up    = normalize(cross(right, front));
			const vec2 off = scroll_off;

			// zoom
			if( ImGui::IsKeyDown(ImGuiKey_LeftAlt) ) {
				const vec2 step = spd_zoom_dist * off;
				float distance = length(diff);
				float newDist = distance * pow(1.01f, step.y);
				newDist = clamp(newDist, MIN_DIST, MAX_DIST);
				position = -newDist/distance * diff + pivot;
			}
			// move tangent plane
			else if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
				const vec3 step = spd_pivot_move * vec3(right*off.x + global_up*off.y); // todo up
				pivot += step;
				position += step;
			}
			// rotate
			else {
				const vec2 step = off * spd_scroll_rot;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(-step.y, -right)*vec4(-diff,0);
				if( abs(rotated.y) < length(rotated)*0.9f )
					position = pivot + rotated;
			}
		}

		if( input_status&(IST_SCROLLED|IST_DRAGGED) ) {
			updateViewMat();
		}
	}
	void CameraController::updateFromInput()
	{
		if( ImGui::IsKeyPressed(ImGuiKey_Tab, false) ) {
			setViewMode(viewing_mode+1);
		}
		switch( viewing_mode ) {
			case VM_PIVOT:  updatePivotMode(); break;
			case VM_FREE:   updateFreeMode(); break;
			case VM_SCROLL:	updateScrollMode(); break;
		}
		input_status = IST_NONE;
	}



	CameraManWin::CameraManWin()
	{
		initCallbacks();
	}
	CameraManWin::CameraManWin(CameraManWin&& src) noexcept
		: CameraController(std::move(src))
	{
		src.deinitCallbacks();
		initCallbacks();
	}
	CameraManWin& CameraManWin::operator=(CameraManWin&& src) noexcept
	{
		if( this==&src )
			return *this;
		CameraController::operator=(std::move(src));

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
		app->scroll_callbacks[this] = [this](double xOff, double yOff) {
			if( ImGui::GetIO().WantCaptureMouse )
				return;
			scroll_off = {xOff, yOff};
		};
		app->update_hooks[this] = [this](float dt) {
			if( scroll_off.x||scroll_off.y ) {
				input_status |= IST_SCROLLED;
			}
			if( (glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS)
				| (glfwGetMouseButton(app->window, GLFW_MOUSE_BUTTON_MIDDLE)==GLFW_PRESS)
				| (!ImGui::GetIO().WantCaptureMouse) ) {
				input_status |= IST_DRAGGED;
			}
			if( (!ImGui::GetIO().WantCaptureMouse) && (!ImGui::GetIO().WantCaptureKeyboard) ) {
				input_status |= IST_FOCUSED;
			}
			mouse_off = AppPref::get().app->mouse_off;
			updateFromInput();
			scroll_off = {0,0};
		};
	}



	CameraManVp::CameraManVp(Viewport* _vp)
	{
		initCallbacks(_vp);
	}
	CameraManVp::CameraManVp(CameraManVp&& src) noexcept
		:CameraController(std::move(src))
	{
		src.deinitCallbacks();
		initCallbacks(src.vp);
	}
	CameraManVp& CameraManVp::operator=(CameraManVp&& src) noexcept
	{
		if( this==&src )
			return *this;
		deinitCallbacks();

		CameraController::operator=(std::move(src));

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
			if( vp->is_scrolled ) {
				input_status |= IST_SCROLLED;
			}
			if( vp->is_dragged ) {
				input_status |= IST_DRAGGED;
			}
			if( vp->is_focused ) {
				input_status |= IST_FOCUSED;
			}
			scroll_off = vp->scroll_off;
			mouse_off = vp->mouse_off;
			updateFromInput();
		};
	}
	void CameraManVp::copySettingTo(CameraManVp& dst)
	{
		dst.fovy = fovy;
		dst.z_near = z_near;
		dst.position = position;
		dst.pivot = pivot;
		dst.global_up = global_up;
		dst.view_mat = view_mat;
		dst.proj_mat = proj_mat;
		dst.viewing_mode = viewing_mode;
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