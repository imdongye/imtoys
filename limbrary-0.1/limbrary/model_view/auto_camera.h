//
//	윈도우 또는 뷰포트와 3가지 모드로 상호작용하는 카메라
// 
//  2023-02-28 / im dongye
// 
//	Todo:
//	1. 화면 크기 비례해서 드레그 속도 조절
//

#ifndef __auto_camera_h_
#define __auto_camera_h_

#include "camera.h"
#include "../viewport.h"
#include <GLFW/glfw3.h>

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
		int viewing_mode;
		GLFWwindow *window;
		Viewport *vp;
	private:
		float move_free_spd = 4.2;
		float move_free_spd_fast = 8.2;
		float rot_free_spd = 0.09;

		float move_pivot_spd = -0.003;
		float rot_pivot_spd = 0.003;

		float rot_pivot_scroll_spd = 0.045;
		float move_pivot_scroll_spd = -0.09;

		float zoom_spd = 1;
		float zoom_dist_spd = 1;

		double cur_x_off = 0;
		double cur_y_off = 0;
		AutoCamera(const AutoCamera&)=delete;
		AutoCamera& operator=(const AutoCamera&) = delete;
	public:
		// if vp is nullptr then interaction with window
		AutoCamera(GLFWwindow* _win, Viewport *_vp = nullptr
				   , int viewingMode = VM_FREE
				   , glm::vec3 _pos = {0,0,8}, glm::vec3 _focus = {0,0,0});
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