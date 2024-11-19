/*
	imdongye@naver.com
	fst: 2022-07-21
	lst: 2022-07-21

Note:
	yaw, pitch, roll 은 free/pivot 모드일때 다르게 사용됨.
	입력은 degree로 사용하고 행렬 만들때 radian으로 변환함.

 	Euler angles : roll(z)-pitch(x)-yaw(y)

	free view (pos and dir) 와 pivot view (track ball, orbit) 지원

Todo:
 	1. quaternion

*/

#ifndef __camera_h_
#define __camera_h_

#include <glm/glm.hpp>
#include <limbrary/program.h>
#include <limbrary/tools/mecro.h>

namespace lim
{
	// copyable
	class Camera : public NoCopyAndMove
	{
	public:
		// <editable camera options>
		// if you edit then must call updateProjMtx();
		float aspect=1;
		float fovy = 46.f; // 50mm, feild of view y axis dir
		float z_near=0.1f;
		float z_far=100;
		
		// todo: TransformPivoted
		// if you edit then must call updateViewMtx();
		glm::vec3 pos = {0,0,5};
		glm::vec3 pivot = {0,0,0};
		glm::vec3 global_up = {0,1,0};

		// result of updateViewMtx
		glm::vec3 to_pivot;
		float distance;
		glm::vec3 front, up, right;
		glm::mat4 mtx_View;
		glm::mat4 mtx_Proj;

		Camera();
		virtual ~Camera() noexcept = default;
		glm::vec3 screenPosToDir(const glm::vec2& uv) const;
		void moveShift(const glm::vec3& off);
		// with pos and pivot
		void viewMtxToPos();
		void updateViewMtx(); // and result
		void updateProjMtx();
		void setUniformTo(const Program& prg) const;
		void copyFrom(const Camera& src);
	};
}
#endif