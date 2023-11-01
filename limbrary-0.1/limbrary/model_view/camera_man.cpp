#include <limbrary/model_view/camera_man.h>
#include <limbrary/asset_lib.h>
#include <limbrary/application.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <imguizmo/ImGuizmo.h>
#include <limbrary/log.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
				const vec3 step = spd_pivot_move * vec3(-right*off.x + up*off.y); // todo up
				moveShift(step);
			}
			// rotate
			else {
				const vec2 step = off * spd_scroll_rot;
				const vec3 rotated = rotate(-step.x, global_up)*rotate(step.y, -right)*vec4(-diff,0);
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

		app = AssetLib::get().app;

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
			mouse_off = AssetLib::get().app->mouse_off;
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
	bool ViewportWithCamera::drawImGui()
	{
		Framebuffer& fb = *framebuffer;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
		ImGui::SetNextWindowSize({(float)fb.width, (float)fb.height+ImGui::GetFrameHeight()}, (window_mode==WM_FIXED_SIZE)?ImGuiCond_Always:ImGuiCond_Once);

		ImGuiWindowFlags vpWinFlag = ImGuiWindowFlags_NoCollapse;
		if( window_mode==WM_FIXED_RATIO ) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
				Viewport& vp = *(Viewport*)(data->UserData);

				float frameHeight = ImGui::GetFrameHeight();
				float aspectedWidth = vp.getAspect() * (data->DesiredSize.y - frameHeight);
				float aspectedHeight = data->DesiredSize.x / vp.getAspect() + frameHeight;

				if( data->DesiredSize.x <= aspectedWidth )
					data->DesiredSize.x = aspectedWidth;
				else
					data->DesiredSize.y = aspectedHeight;
			}, (void*)this);
			vpWinFlag |= ImGuiWindowFlags_NoDocking;
		}
		else if( window_mode==WM_FIXED_SIZE ) {
			vpWinFlag |= ImGuiWindowFlags_NoResize;
		}
		ImGui::Begin(name.c_str(), &is_opened, vpWinFlag);
		
		// for ignore draging
		bool isHoveredOnTitle = ImGui::IsItemHovered(); // when move window
		bool isWindowActivated = ImGui::IsItemActive(); // when resize window

		// update size
		auto contentSize = ImGui::GetContentRegionAvail();
		if( fb.width!=contentSize.x || fb.height !=contentSize.y ) {
			if(contentSize.x>10&&contentSize.y>10) // This is sometimes negative
				resize(contentSize.x, contentSize.y);
		}

		ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
		ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
		GLuint texID = framebuffer->getRenderedTex();
		if( texID!=0 ) {
			ImGui::Image((void*)(intptr_t)texID, ImVec2{(float)fb.width, (float)fb.height}, ImVec2{0, 1}, ImVec2{1, 0});
		}
	
		is_focused = ImGui::IsWindowFocused();
		is_hovered = ImGui::IsItemHovered();
		is_dragged = isWindowActivated && !isHoveredOnTitle && ImGui::IsMouseDown(0);
		is_dragged |= (is_hovered||is_focused)&&(ImGui::IsMouseDown(1)||ImGui::IsMouseDown(2));

		prev_mouse_pos = mouse_pos;
		ImVec2 imMousePos = ImGui::GetMousePos() - ImGui::GetWindowPos() - ImVec2(0, ImGui::GetFrameHeight());
		mouse_pos = {imMousePos.x, imMousePos.y};
		mouse_off = mouse_pos - prev_mouse_pos;
		prev_mouse_pos = mouse_pos;

		ImGuiIO io = ImGui::GetIO();
		is_scrolled =  is_hovered&&(io.MouseWheel||io.MouseWheelH);
		scroll_off = {io.MouseWheelH, io.MouseWheel};

		if(use_guizmo) {
			drawGuizmo();
			is_dragged &= !ImGuizmo::IsUsingAny();
		}
		
		if( is_dragged ) ImGui::SetMouseCursor(7);
		ImGui::End();
		ImGui::PopStyleVar();

		for( auto& cb : update_callbacks ) {
			cb(ImGui::GetIO().DeltaTime);
		}
		return is_opened;
	}
	void ViewportWithCamera::drawGuizmo() {
		const auto& pos = ImGui::GetItemRectMin();
		const auto& size = ImGui::GetItemRectSize();
		glm::mat4 identity{1.f};
		float* modelMat = glm::value_ptr(identity);
		float* viewMat = glm::value_ptr(camera.view_mat);
		float* projMat = glm::value_ptr(camera.proj_mat);
		ImGuizmo::OPERATION zmoOper = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE 		zmoMode = ImGuizmo::WORLD;


		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);
		ImGuizmo::Manipulate( viewMat, projMat, zmoOper, zmoMode, modelMat
							, nullptr, nullptr, nullptr);
		
		ImGuizmo::SetOrthographic(false);

		ImGuizmo::ViewManipulate( glm::value_ptr(camera.view_mat)
								, 8.0f, {pos.x+size.x-128, pos.y}, {128, 128}, 0x10101010);
	}
}