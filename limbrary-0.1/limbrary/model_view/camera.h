//
//	멤버변수로 view projection 생성
// 
//  2022-07-21 / im dongye
//
//  note:
//	yaw, pitch, roll 은 free/pivot 모드일때 다르게 사용됨.
//	입력은 degree로 사용하고 행렬 만들때 radian으로 변환함.
// 
//  Euler angles : roll(z)-pitch(x)-yaw(y)
// 
//	free view (pos and dir) 와 pivot view (track ball, orbit) 지원
//
//  TODO list :
//  1. quaternion
//

#ifndef __camera_h_
#define __camera_h_

#include <glm/glm.hpp>

namespace lim
{
	// copyable
	class Camera
	{
	private:
		const float MAX_FOVY = 120.f;
		const float MIN_FOVY = 20.f;
		const float MAX_DIST = 17.f;
		const float MIN_DIST = 0.1f;
	public:
		// <editable camera options>
		// if you edit then must call updateProjMat();
		float aspect=1;
		float fovy = 46.f; // 50mm, feild of view y axis dir
		float z_near=0.1f;
		float z_far=100;

		// if you edit then must call updateFromPosAndPivot();
		// and updateViewMat();
		glm::vec3 position = {0,0,5};
		glm::vec3 pivot = {0,0,0};

		// <only read>
		float distance = 5;
		glm::vec3 front = {0,0,-1};
		glm::vec3 right = {1,0,0};
		glm::vec3 up = {0,1,0};
		glm::vec3 global_up = {0,1,0};

		glm::mat4 view_mat;
		glm::mat4 proj_mat;
	public:
		Camera();
		virtual ~Camera();
	public:
		void updateFromPosAndPivot();
		void updateProjMat();

		void rotateCamera(float xoff, float yoff);
		void rotateCameraFromPivot(float xoff, float yoff);

		void shiftPos(glm::vec3 off);
		void shiftPosFromPlane(float xoff, float yoff);
		void shiftDist(float offset);
		void shiftZoom(float offset);
	};
}
#endif