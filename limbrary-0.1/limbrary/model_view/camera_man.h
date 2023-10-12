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
	class CameraCtrlData : public Camera
	{
	public:
		const float MAX_FOVY = 120.f;
		const float MIN_FOVY = 20.f;
		const float MAX_DIST = 17.f;
		const float MIN_DIST = 0.1f;

		enum VIEWING_MODE
		{
			VM_FREE=0,
			VM_PIVOT,
			VM_SCROLL
		};
		int viewing_mode = VM_FREE;

		float move_free_spd = 3.f/1.f; 	  // m/sec
		float rot_free_spd = D_PI/1900.f; // r/px

		float move_pivot_spd = 1.f/400.f; // m/px
		float rot_pivot_spd = D_PI/800.f; // r/px

		float move_pivot_scroll_spd = 1.f/100.f; // m/scrollOff
		float rot_pivot_scroll_spd = D_PI/400.f; // r/scrollOff

		float zoom_fovy_spd = 1;
		float zoom_dist_spd = 1;
		
	public:
		CameraCtrlData();
		CameraCtrlData(CameraCtrlData&& src) noexcept;
		CameraCtrlData& operator=(CameraCtrlData&& src) noexcept;
		virtual ~CameraCtrlData() noexcept;
		void setViewMode(int vm);
	};

	class CameraManWin : public CameraCtrlData
	{
	private:
		AppBase* app = nullptr;
		bool is_scrolled = false;
		glm::vec2 scroll_off = {0,0};
	private:
		CameraManWin(const CameraManWin&) = delete;
		CameraManWin& operator=(const CameraManWin&) = delete;
	public:
		CameraManWin();
		CameraManWin(CameraManWin&& src) noexcept;
		CameraManWin& operator=(CameraManWin&& src) noexcept;
		virtual ~CameraManWin() noexcept;
	private:
		void updateFreeMode(float dt);
		void updatePivotMode(float dt);
		void updateScrollMode(float dt);
	private:
		void initCallbacks();
		void deinitCallbacks();
	};
	
	class CameraManVp : public CameraCtrlData
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
	private:
		void updateFreeMode(float dt);
		void updatePivotMode(float dt);
		void updateScrollMode(float dt);
	public:
		void initCallbacks(Viewport* vp);
		void deinitCallbacks();
		void copySettingTo(CameraManVp& cam);
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