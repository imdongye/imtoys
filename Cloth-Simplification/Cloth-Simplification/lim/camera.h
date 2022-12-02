//
//  2022-07-21 / im dongye
//  edit learnopengl code
//
//  noti:
//  기본 -z방향이 뷰방향, roll-pitch-yaw 순서로 
//  fixed-axis rotation(pre multiflication)
// `g => free rotation
//  좌클릭+wasd+qe => 움직임 (shift속도증가)
//  좌클릭+alt+drag => ceter pivot rotation
//  좌클릭+alt+ctrl+drag => center pivot fovy, dist 조정
//  스크롤 => fovy 조정
// 
//	distance = -1 ...
// 
//
//  TODO list :
//  1. roll
//  2. fix zoom and change mode error
//

#ifndef CAMERA_H
#define CAMERA_H


namespace lim
{
	const float SCROLL_SPEED = 1.0f;
	const float MAX_FOVY = 120.f;
	const float MIN_FOVY = 20.f;
	const float MAX_DIST = 17.f;
	const float MIN_DIST = 0.1f;
	class Camera
	{
	public:
		enum class MOVEMENT
		{
			FORWARD,
			BACKWARD,
			LEFT,
			RIGHT,
			UP,
			DOWN
		};
	public:
		// camera options
		float aspect=1;
		float zNear=0.1;
		float zFar=100;
		// editable
		glm::vec3 position;
		float yaw; // degrees
		float pitch;
		float roll=0;
		float fovy = 55; // feild of view y axis dir
		// for pivot view mode
		float distance;
		glm::vec3 pivot;
		// result of inside update
		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;
		glm::mat4 viewMat;
		glm::mat4 projMat;
		//glm::mat4 vpMat;
	public:
		Camera() {}
		Camera(glm::vec3 _position, glm::vec3 _front, float _aspect=1, glm::vec3 _pivot=glm::vec3(0))
			: pivot(_pivot)
		{
			distance = -1;

			position = _position;
			front = normalize(_front);
			updateRotateFromFront();

			aspect = _aspect;
			updateFreeViewMat();
			updateProjMat();
		}
		Camera(float _distance, float _yaw, float _pitch, glm::vec3 _pivot, float _aspect=1)
			:pivot(_pivot)
		{
			distance = _distance;
			yaw = _yaw;
			pitch = _pitch;
			aspect = _aspect;
			updatePivotViewMat(); // for make position
			updateProjMat();
		}
		void move(MOVEMENT direction, float speed, float deltaTime)
		{
			float velocity = speed * deltaTime;

			switch( direction ) {
			case MOVEMENT::FORWARD:   position += front * velocity; break;
			case MOVEMENT::BACKWARD:  position -= front * velocity; break;
			case MOVEMENT::LEFT:      position -= right * velocity; break;
			case MOVEMENT::RIGHT:     position += right * velocity; break;
			case MOVEMENT::UP:        position += glm::vec3(0, 1, 0) * velocity; break;
			case MOVEMENT::DOWN:      position -= glm::vec3(0, 1, 0) * velocity; break;
			}
		}
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
		void updatePivotViewMat()
		{
			static glm::vec3 dir;
			if( distance<0 ) {
				dir = pivot-position;
				distance = glm::length(dir);
				// front => yaw pitch
				updateRotateFromFront();
			}

			// -yaw to match obj ro
			position = glm::vec3(glm::rotate(glm::radians(-yaw), glm::vec3(0, 1, 0))
								 * glm::rotate(glm::radians(pitch), glm::vec3(1, 0, 0))
								 * glm::vec4(0, 0, distance, 1)) + pivot;
			position.y = glm::max(position.y, 0.0f);
			viewMat = glm::lookAt(position, pivot, glm::vec3(0, 1, 0));

			dir = pivot-position;
			front = glm::normalize(dir);
			right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
			up    = glm::normalize(glm::cross(right, front));
		}
		void updateFreeViewMat()
		{
			if( distance>0 ) distance = -1;
			// roll-pitch-yaw
			// fixed(world) basis => pre multiplication
			// https://answers.opencv.org/question/161369/retrieve-yaw-pitch-roll-from-rvec/
			// 
			// 기본 뷰방향을 -z로 봤을때
			glm::vec3 f;
			f.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
			f.y = sin(glm::radians(pitch));
			f.z = -cos(glm::radians(pitch)) * cos(glm::radians(yaw));

			front = glm::normalize(f);
			right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
			up    = glm::normalize(glm::cross(right, front));

			viewMat = glm::lookAt(position, position + front, glm::vec3(0, 1, 0)); // todo: roll => edit up
		}
		void updateProjMat()
		{
			projMat = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
		}
	private:
		void updateRotateFromFront()
		{
			pitch = glm::degrees(asin(front.y));
			//yaw = glm::degrees(acos(-front.z/cos(glm::radians(pitch))));
			yaw = glm::degrees(asin(front.x/cos(glm::radians(pitch))));
		}
		void updateVPMat()
		{
			//vpMat = viewMat*projMat;
		}
		void printCameraState()
		{
			Logger::get().log("PYR  : %f.2, %f.2, %f.2\n", pitch, yaw, roll);
			Logger::get().log("POS  : %f.2, %f.2, %f.2\n", position.x, position.y, position.z);
			Logger::get().log("DIST : %f\n", distance);
		}
	};
} // namespace lim
#endif