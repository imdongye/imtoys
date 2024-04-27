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
using namespace lim;


Camera::Camera()
{
	updateViewMat();
	updateProjMat();
}
Camera::~Camera()
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