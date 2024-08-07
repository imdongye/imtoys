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
#include <limbrary/program.h>

namespace lim
{
	// copyable
	class Camera
	{
	public:
		// <editable camera options>
		// if you edit then must call updateProjMat();
		float aspect=1;
		float fovy = 46.f; // 50mm, feild of view y axis dir
		float z_near=0.1f;
		float z_far=100;
		
		// todo: TransformPivoted
		// if you edit then must call updateViewMat();
		glm::vec3 pos = {0,0,5};
		glm::vec3 pivot = {0,0,0};
		glm::vec3 global_up = {0,1,0};

		// result
		float length;
		glm::vec3 front, up, right;
		glm::mat4 mtx_View;
		glm::mat4 mtx_Proj;
	public:
		Camera(const Camera&)	         = delete;
		Camera(Camera&&)			     = delete;
		Camera& operator=(const Camera&) = delete;
		Camera& operator=(Camera&&)      = delete;

		Camera();
		virtual ~Camera();
		glm::vec3 screenPosToDir(const glm::vec2& uv) const;
		void moveShift(const glm::vec3& off);
		// with pos and pivot
		void updateViewMat();
		void updateProjMat();
		void setUniformTo(const Program& prg) const;
		void copyFrom(const Camera& src);
	};
}
#endif