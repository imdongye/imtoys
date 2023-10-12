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

#include <limbrary/model_view/camera.h>
#include <glm/gtx/transform.hpp>
#include <limbrary/log.h>

using namespace glm;

namespace lim
{
	Camera::Camera()
	{
		updateViewMat();
		updateProjMat();
	}
	Camera::Camera(Camera&& src) noexcept
	{
		*this = std::move(src);
	}
	Camera& Camera::operator=(Camera&& src) noexcept
	{
		if( this==&src )
			return *this;
		aspect = src.aspect;
		fovy = src.fovy;
		z_near = src.z_near;
		z_far = src.z_far;
		position = src.position;
		pivot = src.pivot;
		global_up = src.global_up;
		view_mat = src.view_mat;
		proj_mat = src.proj_mat;
		return *this;
	}
	Camera::~Camera() noexcept
	{
	}
	void Camera::moveShift(const glm::vec3& off) {
		position += off;
		pivot += off;
	}
	void Camera::updateViewMat()
	{
		view_mat = lookAt(position, pivot, global_up);
	}
	void Camera::updateProjMat()
	{
		proj_mat = perspective(radians(fovy), aspect, z_near, z_far);
	}
}