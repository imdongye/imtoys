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
#include <glm/gtx/extended_min_max.hpp>

namespace lim
{
	Camera::Camera(glm::vec3 _position, glm::vec3 _front, float _aspect)
		: position(_position), front(glm::normalize(_front)), aspect(_aspect)
	{
		position = _position;
		updateOrientationFromFront();

		updateFreeViewMat();
		updateProjMat();
	}

	void Camera::rotateCamera(float xoff, float yoff)
	{
		yaw += xoff;
		pitch = glm::clamp(pitch+yoff, -89.f, 89.f);
	}
	void Camera::shiftPos(float xoff, float yoff)
	{
		glm::vec3 shift = up*yoff+right*xoff;
		pivot += shift;
		position += shift;
	}
	void Camera::shiftDist(float offset)
	{
		distance = distance * glm::pow(1.01f, offset);
		distance = glm::clamp(distance, MIN_DIST, MAX_DIST);
	}
	void Camera::shiftZoom(float offset)
	{
		fovy = fovy * glm::pow(1.01f, offset);
		fovy = glm::clamp(fovy, MIN_FOVY, MAX_FOVY);
	}

	void Camera::updateFreeViewMat()
	{
		// roll-pitch-yaw
		// fixed(world) basis => pre multiplication
		// https://answers.opencv.org/question/161369/retrieve-yaw-pitch-roll-from-rvec/
		glm::vec3 f;
		f.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		f.y = sin(glm::radians(pitch));
		f.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw));

		front = glm::normalize(f);
		right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
		up    = glm::normalize(glm::cross(right, front));

		view_mat = glm::lookAt(position, position + front, glm::vec3(0, 1, 0));
	}
	void Camera::updatePivotViewMat()
	{
		static glm::vec3 dir;

		// -yaw to match obj ro
		position = glm::vec3(glm::rotate(glm::radians(-yaw), glm::vec3(0, 1, 0))
								* glm::rotate(glm::radians(pitch), glm::vec3(1, 0, 0))
								* glm::vec4(0, 0, distance, 1)) + pivot;
		position.y = glm::max(position.y, 0.0f);
		view_mat = glm::lookAt(position, pivot, glm::vec3(0, 1, 0));

		dir = pivot-position;
		front = glm::normalize(dir);
		right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
		up    = glm::normalize(glm::cross(right, front));
	}
	void Camera::updateProjMat()
	{
		proj_mat = glm::perspective(glm::radians(fovy), aspect, z_near, z_far);
	}
	void Camera::updateOrientationFromFront()
	{
		pitch = glm::degrees(asin(front.y));
		//yaw = glm::degrees(acos(-front.z/cos(glm::radians(pitch))));
		yaw = glm::degrees(asin(front.x/cos(glm::radians(pitch))));
	}
	void Camera::printCameraState()
	{
		printf("PitchYawRoll  : %f.2, %f.2, %f.2\n", pitch, yaw, roll);
		printf("POS  : %f.2, %f.2, %f.2\n", position.x, position.y, position.z);
		printf("DIST : %f\n", distance);
	}
}