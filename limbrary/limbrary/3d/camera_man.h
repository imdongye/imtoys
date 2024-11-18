/*

2023-02-28 / im dongye

Note:
	camera ctrl with three modes
	
	1. free mode
	2. track ball mode
	3. scroll track ball mode


	camera man have to 

	1. set input_status, scroll_off, mouse_off and call updateViewMatWithCtrlMember
	2. updateProjMtx


	user have to call updateFrom()


Todo:
	1. initial framebuffer size setting
	2. drag imgui demo 참고해서 다시짜기
	3. https://github.com/ocornut/imgui/issues/3152
		https://github.com/ocornut/imgui/issues/3492
		https://jamssoft.tistory.com/234
		https://github.com/ocornut/imgui/blob/master/docs/FAQ.md
		https://github.com/ocornut/imgui/issues/270
		로 앱별 tag부여해서 ini 윈도우 설정 겹치지 않게


*/

#ifndef __camera_man_h_
#define __camera_man_h_

#include "camera.h"
#include "../application.h"
#include "../viewport.h"


namespace lim
{
	class CameraCtrl : public Camera
	{
	public:
		float max_fovy;
		float min_fovy;
		float max_dist;
		float min_dist;

		float spd_fovy;
		float spd_dist;
		float spd_free_move;
		float spd_free_move_fast;
		float spd_free_rot;
		float spd_tb_mouse_move;
		float spd_tb_mouse_rot;
		float spd_tb_scroll_move;
		float spd_tb_scroll_rot;
	
		inline static constexpr int nr_viewing_modes = 3;
		enum ViewingMode : int {
			VM_FREE=0,
			VM_TRACKBALL_MOVE,
			VM_TRACKBALL_SCROLL
		};
		ViewingMode viewing_mode = VM_FREE;

		bool enabled = true;

	public:
		CameraCtrl();
		virtual ~CameraCtrl() noexcept = default;

	private:
		// for press a and d key in same time in free mode
		bool is_left = false;
		bool prev_is_left = false;
		void updateFreeMode(const glm::vec3& moveOff, const glm::vec2& rotOff, float fovyOff);
		void updateTrackballMode(const glm::vec2& moveOff, const glm::vec2& rotOff, float fovyOff, float distOff);
	
	protected:
		// call in drawImgui
		void update(const glm::vec2& dragOff, const glm::vec2& scrollOff, bool wantCaptureKey);
	};



	class CameraManWin : public CameraCtrl
	{
	public:
		CameraManWin() = default;
		virtual ~CameraManWin() noexcept = default;
		void updateFrom(AppBase& app);
	};

	class CameraManVp : public CameraCtrl
	{
	public:
		CameraManVp() = default;
		virtual ~CameraManVp() noexcept = default;
		void updateFrom(Viewport& vp);
	};
}

#endif