/*

2023-02-28 / im dongye

3가지 모드로 윈도우와 상호작용하는 카메라

Note:
* VpAutoCamera control setting is same as AutoCamera
  so you must edit code togather

Todo:
1. 화면 크기 비례해서 드레그 속도 조절


*/

#ifndef __auto_camera_h_
#define __auto_camera_h_

#include "camera.h"
#include "../viewport.h"

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
		int viewing_mode = VM_FREE;

		float move_free_spd = 3.f/1.f; 	  // m/sec
		float rot_free_spd = D_PI/1600.f; // r/px

		float move_pivot_spd = 1.f/400.f; // m/px
		float rot_pivot_spd = D_PI/800.f; // r/px

		float move_pivot_scroll_spd = 1.f/100.f; // m/scrollOff
		float rot_pivot_scroll_spd = D_PI/400.f; // r/scrollOff

		float zoom_fovy_spd = 1;
		float zoom_dist_spd = 1;
	private:
		double prev_mouse_x = 0;
		double prev_mouse_y = 0;
	private:
		AutoCamera(const AutoCamera&) = delete;
		AutoCamera& operator=(const AutoCamera&) = delete;
	public:
		AutoCamera();
		virtual ~AutoCamera();
	public:
		void setViewMode(int vm);
	protected:
		void keyCallback(int key, int scancode, int action, int mods);
		void viewportSizeCallback(int w, int h);
		void mouseBtnCallback(int button, int action, int mods);
		void cursorPosCallback(double xpos, double ypos);
		void scrollCallback(double xOff, double yOff);
		void processInput(float dt);
	};

	class WinAutoCamera : public AutoCamera
	{
	private:
		WinAutoCamera(const WinAutoCamera&) = delete;
		WinAutoCamera& operator=(const WinAutoCamera&) = delete;
	public:
		WinAutoCamera();
		virtual ~WinAutoCamera();
	};
	
	class VpAutoCamera : public AutoCamera
	{
	private:
		VpAutoCamera(const VpAutoCamera&) = delete;
		VpAutoCamera& operator=(const VpAutoCamera&) = delete;
	public:
		Viewport* vp;
		VpAutoCamera(Viewport* vp);
		virtual ~VpAutoCamera();
		void copySettingTo(VpAutoCamera& cam);
	};
}

#endif