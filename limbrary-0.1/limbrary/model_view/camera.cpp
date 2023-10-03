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
		updateFromPosAndPivot();
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
		distance = src.distance;
		front = src.front;
		right = src.right;
		up = src.up;
		global_up = src.global_up;
		view_mat = src.view_mat;
		proj_mat = src.proj_mat;
		return *this;
	}
	Camera::~Camera() noexcept
	{
	}
	

	void Camera::rotateCamera(float xoff, float yoff)
	{
		vec3 rotated = rotate(yoff, right)*rotate(-xoff, up)*vec4(front,0);
		pivot = distance*rotated + position;
		updateFromPosAndPivot();
	}
	void Camera::rotateCameraFromPivot(float xoff, float yoff)
	{
		vec3 toCam = -front;
		vec3 rotated = rotate(-yoff, -right)*rotate(-xoff, global_up)*vec4(toCam,0);
		if( abs(rotated.y)>0.9f )
			return;
		position = pivot + distance*rotated;
		updateFromPosAndPivot();
	}
	void Camera::shiftPos(glm::vec3 off)
	{
		pivot += off;
		position += off;
		view_mat = lookAt(position, pivot, global_up);
	}
	void Camera::shiftPosFromPlane(float xoff, float yoff)
	{
		vec3 shift = global_up * yoff + right * xoff;
		pivot += shift;
		position += shift;
		view_mat = lookAt(position, pivot, global_up);
	}
	void Camera::shiftDist(float offset)
	{
		distance = distance * pow(1.01f, offset);
		distance = clamp(distance, MIN_DIST, MAX_DIST);
		position = -distance*front + pivot;
		view_mat = lookAt(position, pivot, global_up);
	}
	void Camera::shiftZoom(float offset)
	{
		fovy = fovy * pow(1.01f, offset);
		fovy = clamp(fovy, MIN_FOVY, MAX_FOVY);
		updateProjMat();
	}

	void Camera::updateFromPosAndPivot()
	{
		vec3 diff = pivot - position;
		distance = length(diff);

		front = normalize(diff);
		right = normalize(cross(front,global_up));
		up = normalize(cross(right, front));
		view_mat = lookAt(position, pivot, global_up);
	}
	void Camera::updateProjMat()
	{
		proj_mat = perspective(radians(fovy), aspect, z_near, z_far);
	}
}