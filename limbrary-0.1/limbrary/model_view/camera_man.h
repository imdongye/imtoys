/*

2023-02-28 / im dongye

3가지 모드로 윈도우와 상호작용하는 카메라

Note:
* CameraManVp control setting is same as AutoCamera
  so you must edit code togather
* Viewport AutoCamera는 Viewport delete하기전에 delete해야함.

Todo:
1. 화면 크기 비례해서 드레그 속도 조절


viewport with camera_man

Note:
* CameraManVp control setting is same as AutoCamera
  so you must edit code togather

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
	// copyable
	class CameraController : public Camera
	{
	public:
		const float MAX_FOVY = 120.f;
		const float MIN_FOVY = 20.f;
		const float MAX_DIST = 17.f;
		const float MIN_DIST = 0.1f;

		float spd_free_move = 2.5f/1.f;    // m/sec
		float spd_free_rot = D_PI/1900.f;  // r/px

		float spd_pivot_move = 1.f/120.f;  // m/px
		float spd_pivot_rot = D_PI/1000.f; // r/px

		float spd_scroll_move = 1.f/100.f; // m/scrollOff
		float spd_scroll_rot = D_PI/400.f; // r/scrollOff
	#ifdef WIN32
		float spd_zoom_fovy = 1;
		float spd_zoom_dist = 6.f;
	#else
		float spd_zoom_fovy = 1;
		float spd_zoom_dist = 3.5f;
	#endif
	
		enum VIEWING_MODE {
			VM_FREE=0,
			VM_PIVOT,
			VM_SCROLL
		};
	protected:
		int viewing_mode = VM_FREE;

		enum INPUT_STATUS {
			IST_NONE     = 1<<0,
			IST_SCROLLED = 1<<1,
			IST_FOCUSED  = 1<<2,
			IST_DRAGGED  = 1<<3
		};
		int input_status = IST_NONE;
		glm::vec2 scroll_off = {0,0};
		glm::vec2 mouse_off = {0,0};
	private:
		// for a and b key simultaneous input
		bool is_left = false;
		bool prev_is_left = false;
	public:
		CameraController();
		CameraController(CameraController&& src) noexcept;
		CameraController& operator=(CameraController&& src) noexcept;
		virtual ~CameraController() noexcept;
		void setViewMode(int vm);
		void updateFromInput();
	private:
		void updateFreeMode();
		void updatePivotMode();
		void updateScrollMode();
	};

	class CameraManWin : public CameraController
	{
	private:
		AppBase* app = nullptr;
	private:
		CameraManWin(const CameraManWin&) = delete;
		CameraManWin& operator=(const CameraManWin&) = delete;
	public:
		CameraManWin();
		CameraManWin(CameraManWin&& src) noexcept;
		CameraManWin& operator=(CameraManWin&& src) noexcept;
		virtual ~CameraManWin() noexcept;
	private:
		void initCallbacks();
		void deinitCallbacks();
	};
	
	class CameraManVp : public CameraController
	{
	private:
		Viewport* vp = nullptr;
	private:
		CameraManVp(const CameraManVp&) = delete;
		CameraManVp& operator=(const CameraManVp&) = delete;
	public:
		CameraManVp(Viewport* vp=nullptr);
		CameraManVp(CameraManVp&& src) noexcept;
		CameraManVp& operator=(CameraManVp&& src) noexcept;
		virtual ~CameraManVp() noexcept;
	public:
		void initCallbacks(Viewport* vp);
		void deinitCallbacks();
		void copySettingTo(CameraManVp& dst);
	};



	class ViewportWithCamera : public Viewport
	{
	public:
		CameraManVp camera;
	private:
		ViewportWithCamera(const ViewportWithCamera&) = delete;
		ViewportWithCamera& operator=(const ViewportWithCamera&) = delete;
	public:
		ViewportWithCamera(std::string_view _name, Framebuffer* createdFB);
		ViewportWithCamera(ViewportWithCamera&& src) noexcept;
		ViewportWithCamera& operator=(ViewportWithCamera&& src) noexcept;
		virtual ~ViewportWithCamera() noexcept;
	};
}

#endif