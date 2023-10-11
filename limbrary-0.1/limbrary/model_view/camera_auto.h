/*

2023-02-28 / im dongye

3가지 모드로 윈도우와 상호작용하는 카메라

Note:
* VpAutoCamera control setting is same as AutoCamera
  so you must edit code togather

Todo:
1. 화면 크기 비례해서 드레그 속도 조절


viewport with camera_auto

Note:
* VpAutoCamera control setting is same as AutoCamera
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

#ifndef __camera_auto_h_
#define __camera_auto_h_

#include "camera.h"
#include "../viewport.h"

namespace lim
{
	// copyable
	class AutoCamera : public Camera
	{
	public:
		enum VIEWING_MODE
		{
			VM_FREE=0,
			VM_PIVOT,
			VM_SCROLL
		};
		bool is_viewing_mode_key_down = false;
		int viewing_mode = VM_FREE;

		float move_free_spd = 3.f/1.f; 	  // m/sec
		float rot_free_spd = D_PI/1900.f; // r/px

		float move_pivot_spd = 1.f/400.f; // m/px
		float rot_pivot_spd = D_PI/800.f; // r/px

		float move_pivot_scroll_spd = 1.f/100.f; // m/scrollOff
		float rot_pivot_scroll_spd = D_PI/400.f; // r/scrollOff

		float zoom_fovy_spd = 1;
		float zoom_dist_spd = 1;
	private:
		double prev_mouse_x = 0;
		double prev_mouse_y = 0;
	public:
		AutoCamera();
		AutoCamera(AutoCamera&& src) noexcept;
		AutoCamera& operator=(AutoCamera&& src) noexcept;
		virtual ~AutoCamera() noexcept;
	public:
		void setViewMode(int vm);
	protected:
		void viewportSizeCallback(int w, int h);
		void cursorPosCallback(double xpos, double ypos, bool isDragging);
		void scrollCallback(double xOff, double yOff);
		void processInput(float dt);
	};

	class WinAutoCamera : public AutoCamera
	{
	private:
		WinAutoCamera(const WinAutoCamera&) = delete;
		WinAutoCamera& operator=(const WinAutoCamera&) = delete;
		void initCallbacks();
		void deinitCallbacks();
		bool isInited = false;
	public:
		WinAutoCamera();
		WinAutoCamera(WinAutoCamera&& src) noexcept;
		WinAutoCamera& operator=(WinAutoCamera&& src) noexcept;
		virtual ~WinAutoCamera() noexcept;
	};
	
	class VpAutoCamera : public AutoCamera
	{
	private:
		Viewport* vp = nullptr;
	private:
		VpAutoCamera(const VpAutoCamera&) = delete;
		VpAutoCamera& operator=(const VpAutoCamera&) = delete;
	public:
		VpAutoCamera(Viewport* vp=nullptr);
		VpAutoCamera(VpAutoCamera&& src) noexcept;
		VpAutoCamera& operator=(VpAutoCamera&& src) noexcept;
		virtual ~VpAutoCamera() noexcept;

		void copySettingTo(VpAutoCamera& cam);
		void initCallbacks(Viewport* vp);
		void deinitCallbacks();
		friend class ViewportWithCamera;
	};



	class ViewportWithCamera : public Viewport
	{
	public:
		VpAutoCamera camera;
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