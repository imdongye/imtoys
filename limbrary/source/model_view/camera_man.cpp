#include <limbrary/model_view/camera_man.h>
#include <limbrary/application.h>

#include <limbrary/tools/limgui.h>
#include <imgui_internal.h>
#include <imguizmo/ImGuizmo.h>
#include <limbrary/tools/log.h>
#include <glm/gtx/transform.hpp>

#include <limbrary/tools/glim.h>
#include <limbrary/using_in_cpp/glm.h>
using namespace lim;


CameraCtrl::CameraCtrl()
{
	max_fovy = 120.f;
	min_fovy = 20.f;
	max_dist = 17.f;
	min_dist = 0.1f;
#ifdef WIN32
	spd_fovy = 1;
	spd_dist = 6.f;
#else
	spd_fovy = 1;
	spd_dist = 3.5f;
#endif
	spd_free_move = 2.f/1.f;    // m/sec
	spd_free_move_fast = 4.f/1.f;    // m/sec
	spd_free_rot = glim::pi2/2100.f;  // r/px
	spd_tb_mouse_move = 1.f/120.f;  // m/px
	spd_tb_mouse_rot = glim::pi2/1000.f; // r/px
	spd_tb_scroll_move = 1.f/100.f; // m/scrollOff
	spd_tb_scroll_rot = glim::pi2/400.f; // r/scrollOff
}

void CameraCtrl::updateFreeMode(const vec3& moveOff, const glm::vec2& rotOff, float fovyOff)
{
	// zoom
	if( fovyOff!=0.f ) {
		fovy = fovy * pow(1.01f, fovyOff);
		fovy = glm::clamp(fovy, min_fovy, max_fovy);
		updateProjMtx();
	}

	bool need_view_mtx_update = false;

	if( glim::isNotZero(moveOff) ) {
		moveShift(moveOff);
		need_view_mtx_update = true;
	}

	// rotate
	if( glim::isNotZero(rotOff) ) {
		vec3 rotated = rotate(-rotOff.x, up)*rotate(-rotOff.y, right)*vec4(to_pivot, 0);
		pivot = rotated + pos;
		need_view_mtx_update = true;
	}

	if( need_view_mtx_update ) {
		updateViewMtx();
	}
}


void CameraCtrl::updateTrackballMode(const vec2& moveOff, const vec2& rotOff, float fovyOff, float distOff)
{
	bool need_view_mtx_update = false;

	if( distOff!=0.f ) {
		float newDist = distance * pow(1.01f, distOff);
		newDist = glm::clamp(newDist, min_dist, max_dist);
		pos = -newDist/distance * to_pivot + pivot;
		need_view_mtx_update = true;
	}
	else if( fovyOff!=0.f ) {
		fovy = fovy * pow(1.01f, fovyOff);
		fovy = glm::clamp(fovy, min_fovy, max_fovy);
		updateProjMtx();
	}


	if( glim::isNotZero(rotOff) ) {
		const vec3 rotated = glm::rotate(-rotOff.x, global_up)*glm::rotate(rotOff.y, -right)*vec4(-to_pivot,0);
		if( abs(rotated.y) < glm::length(rotated)*0.99f ) {
			pos = pivot + rotated;
			need_view_mtx_update = true;
		}
	}
	// move tangent plane
	else if( glim::isNotZero(moveOff) ) {
		const vec3 step = vec3(up*moveOff.y -right*moveOff.x);
		moveShift(step);
		need_view_mtx_update = true;
	}

	if( need_view_mtx_update ) {
		updateViewMtx();
	}
}


void CameraCtrl::update(const vec2& dragOff, const vec2& scrollOff, bool wantCaptureKey)
{
	if( !enabled ) {
		return;
	}
	const ImGuiIO& io = ImGui::GetIO();

	// switch view mode
	if( wantCaptureKey && ImGui::IsKeyPressed(ImGuiKey_Tab, false) ) {
		int ivm = (int(viewing_mode)+1)%nr_viewing_modes;
		viewing_mode = (ViewingMode)ivm;
	}

	if( viewing_mode == VM_FREE )
	{
		const vec3 forwardDir = normalize(vec3{front.x, 0.f, front.z}); // move horizontally
		float moveSpd = spd_free_move;
		vec3 moveOff(0.f);
		if( wantCaptureKey ) {
			vec3 dir(0.f);
			if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) { moveSpd = spd_free_move_fast; }
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
			if( glim::isNotZero(dir) ) {
				moveOff = normalize(dir)*moveSpd*io.DeltaTime;
				prev_is_left = is_left;
			}
		}
		updateFreeMode( moveOff, spd_free_rot*dragOff, spd_fovy*scrollOff.y );
	}

	else if( viewing_mode == VM_TRACKBALL_MOVE )
	{
		if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
			updateTrackballMode(
				spd_tb_mouse_move * dragOff,
				vec2(0.f),
				spd_fovy * scrollOff.y,
				0.f
			);
		}
		else {
			updateTrackballMode(
				vec2(0.f),
				spd_tb_mouse_rot * dragOff,
				0.f,
				spd_dist * scrollOff.y
			);
		}
	}

	else if( viewing_mode == VM_TRACKBALL_SCROLL )
	{
		// Todo: pinch zoom
		if( ImGui::IsKeyDown(ImGuiKey_LeftShift) ) {
			updateTrackballMode(
				spd_tb_scroll_move * scrollOff,
				vec2(0.f), 0.f, 0.f
			);
		}
		else {
			updateTrackballMode(
				vec2(0.f),
				spd_tb_mouse_rot * dragOff,
				0.f, 0.f
			);
		}
	}
}




void CameraManWin::updateFrom(AppBase& app)
{
	const ImGuiIO& io = ImGui::GetIO();
	if( app.is_size_changed ) {
		aspect = app.aspect_ratio;
		updateProjMtx();
	}
	if( app.is_focused )
	{
		vec2 dragOff = (io.MouseDown[0]) ? toGlm(io.MouseDelta) : vec2(0.f);
		update({io.MouseWheelH, io.MouseWheel}, dragOff, true);
	}
}





void CameraManVp::updateFrom(Viewport& vp)
{
	if( vp.is_size_changed ) {
		aspect = vp.getFb().aspect;
		updateProjMtx();
	}
	const ImGuiIO& io = ImGui::GetIO();
	vec2 dragOff = vp.is_dragged ? toGlm(io.MouseDelta) : vec2(0.f);
	vec2 scrollOff = vp.is_hovered ? vec2(io.MouseWheelH, io.MouseWheel) : vec2(0.f);
	update(dragOff, scrollOff, vp.is_focused);
}
