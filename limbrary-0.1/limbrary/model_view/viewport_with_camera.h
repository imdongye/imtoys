/*

2023-09-08 / im dong ye

viewport with auto_camera


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

#ifndef __viewport_with_camera_h_
#define __viewport_with_camera_h_

#include "../viewport.h"
#include "camera.h"


namespace lim
{
    class VpAutoCamera : public Camera
	{
	public:
		enum VIEWING_MODE
		{
			VM_FREE=0,
			VM_PIVOT,
			VM_SCROLL
		};
		int viewing_mode = VM_FREE;
		Viewport* vp;

		float move_free_spd = 4.2f;
		float rot_free_spd = 0.09f;

		float move_pivot_spd = -0.003f;
		float rot_pivot_spd = 0.003f;

		float rot_pivot_scroll_spd = 0.045f;
		float move_pivot_scroll_spd = -0.09f;

		float zoom_spd = 1;
		float zoom_dist_spd = 1;

	private:
		double prev_mouse_x = 0;
		double prev_mouse_y = 0;
		
	public:
		VpAutoCamera(Viewport *_vp, glm::vec3 _pos = {0,0,8}, glm::vec3 _focus = {0,0,0});
		virtual ~VpAutoCamera();
	public:
		void setViewMode(int vm);
		void copySettingTo(VpAutoCamera& cam);
	private:
		void keyCallback(int key, int scancode, int action, int mods);
		void viewportSizeCallback(int w, int h);
		void mouseBtnCallback(int button, int action, int mods);
		void cursorPosCallback(double xpos, double ypos);
		void scrollCallback(double xOff, double yOff);
		void processInput(float dt);
	};

	class ViewportWithCamera : public Viewport
	{
	public:
		VpAutoCamera camera;
	public:
		ViewportWithCamera(std::string_view _name, Framebuffer* createdFB);
		virtual ~ViewportWithCamera();
	};
}

#endif



