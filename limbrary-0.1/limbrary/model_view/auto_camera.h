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
		AutoCamera(glm::vec3 _pos = {0,0,8}, glm::vec3 _focus = {0,0,0});
		virtual ~AutoCamera();
	public:
		void setViewMode(int vm);
	private:
		void keyCallback(int key, int scancode, int action, int mods);
		void viewportSizeCallback(int w, int h);
		void mouseBtnCallback(int button, int action, int mods);
		void cursorPosCallback(double xpos, double ypos);
		void scrollCallback(double xOff, double yOff);
		void processInput(float dt);
	};
}

#endif