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

#ifndef CAMERA_H
#define CAMERA_H

namespace lim
{
	class Camera
	{
	private:
		const float MAX_FOVY = 120.f;
		const float MIN_FOVY = 20.f;
		const float MAX_DIST = 17.f;
		const float MIN_DIST = 0.1f;
	public:
		// <editable camera options>
		float aspect=1;
		float z_near=0.1;
		float z_far=100;

		glm::vec3 position;
		float orientation;
		float yaw, pitch, roll;

		float fovy = 46.f; // 50mm, feild of view y axis dir
		/* for pivot view mode */
		float distance;
		glm::vec3 pivot= {0,0,0};

		// <result of inside update>
		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;
		glm::mat4 view_mat;
		glm::mat4 proj_mat;
	public:
		// free view (pos and dir)
		Camera(glm::vec3 _position, glm::vec3 _front= {0,0,-1}, float _aspect=1)
			: position(_position), front(glm::normalize(_front)), aspect(_aspect)
		{
			position = _position;
			updateOrientationFromFront();

			updateFreeViewMat();
			updateProjMat();
		}
		virtual ~Camera() {}
	public:
		void rotateCamera(float xoff, float yoff)
		{
			yaw += xoff;
			pitch = glm::clamp(pitch+yoff, -89.f, 89.f);
		}
		void shiftPos(float xoff, float yoff)
		{
			glm::vec3 shift = up*yoff+right*xoff;
			pivot += shift;
			position += shift;
		}
		void shiftDist(float offset)
		{
			distance *= pow(1.01, offset);
			distance = glm::clamp(distance, MIN_DIST, MAX_DIST);
		}
		void shiftZoom(float offset)
		{
			fovy = fovy * pow(1.01, offset);
			fovy = glm::clamp(fovy, MIN_FOVY, MAX_FOVY);
		}
	public:
		void updateFreeViewMat()
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
		void updatePivotViewMat()
		{
			static glm::vec3 dir;

			// -yaw to match obj ro
			position = glm::vec3(glm::rotate(glm::radians(-yaw), glm::vec3(0, 1, 0))
									* glm::rotate(glm::radians(pitch), glm::vec3(1, 0, 0))
									* glm::vec4(0, 0, distance, 1)) + pivot;
			position.y = std::max(position.y, 0.0f);
			view_mat = glm::lookAt(position, pivot, glm::vec3(0, 1, 0));

			dir = pivot-position;
			front = glm::normalize(dir);
			right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
			up    = glm::normalize(glm::cross(right, front));
		}
		void updateProjMat()
		{
			proj_mat = glm::perspective(glm::radians(fovy), aspect, z_near, z_far);
		}
		void updateOrientationFromFront()
		{
			pitch = glm::degrees(asin(front.y));
			//yaw = glm::degrees(acos(-front.z/cos(glm::radians(pitch))));
			yaw = glm::degrees(asin(front.x/cos(glm::radians(pitch))));
		}
		void printCameraState()
		{
			printf("PitchYawRoll  : %f.2, %f.2, %f.2\n", pitch, yaw, roll);
			printf("POS  : %f.2, %f.2, %f.2\n", position.x, position.y, position.z);
			printf("DIST : %f\n", distance);
		}
	};
} // namespace lim
#endif