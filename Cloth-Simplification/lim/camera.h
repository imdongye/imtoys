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
		enum class MODE
		{
			FREE,
			PIVOT
		};
	public:
		MODE mode;
		// camera options
		float aspect;
		float zNear;
		float zFar;
		// editable
		glm::vec3 position;
		float yaw; // degrees
		float pitch;
		float roll;
		float fovy; // feild of view y axis dir
		float distance;
		// result of inside update
		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;
		glm::mat4 viewMat;
		glm::mat4 projMat;
		glm::mat4 vpMat;
	public:
		Camera() {}
		Camera(glm::vec3 _position, glm::vec3 _front, float _aspect)
			: fovy(60), roll(0), zNear(0.1), zFar(100.f), mode(MODE::FREE)
		{

			position = _position;
			front = normalize(_front);
			updateRotateFromFront();

			aspect = _aspect;
			distance = position.length();
			updateFreeViewMat();
			updateProjMat();
		}
		Camera(float _distance, float _yaw, float _pitch, float _aspect)
			: fovy(60), roll(0), zNear(0.1), zFar(100.f), mode(MODE::PIVOT)
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

			if( mode==MODE::FREE ) {
				switch( direction ) {
				case MOVEMENT::FORWARD:   position += front * velocity; break;
				case MOVEMENT::BACKWARD:  position -= front * velocity; break;
				case MOVEMENT::LEFT:      position -= right * velocity; break;
				case MOVEMENT::RIGHT:     position += right * velocity; break;
				case MOVEMENT::UP:        position += glm::vec3(0, 1, 0) * velocity; break;
				case MOVEMENT::DOWN:      position -= glm::vec3(0, 1, 0) * velocity; break;
				}
				updateFreeViewMat();
			}
			else if( mode==MODE::PIVOT ) {
				switch( direction ) {
				case MOVEMENT::FORWARD:   shiftDist(-velocity*100); break;
				case MOVEMENT::BACKWARD:  shiftDist(velocity*100); break;
				}
				updatePivotViewMat();
			}
		}
		void readyPivot()
		{
			mode = MODE::PIVOT;
			distance = glm::length(position);
			front = glm::normalize(-position);
			// front => yaw pitch
			updateRotateFromFront();
		}
		void readyFree()
		{
			mode = MODE::FREE;
			front = glm::normalize(-position);
			updateRotateFromFront();
			printCameraState();
		}
		void rotateCamera(float xoff, float yoff)
		{
			rotateCameraYaw(xoff);
			rotateCameraPitch(yoff);
		}
		void rotateCameraYaw(float yaw_off)
		{
			yaw += yaw_off;//todo repeat
		}
		void rotateCameraPitch(float pitch_off)
		{
			pitch += pitch_off;
			pitch = glm::clamp(pitch, -89.f, 89.f);
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
		void updateRotateFromFront()
		{
			pitch = glm::degrees(asin(front.y));
			//yaw = glm::degrees(acos(-front.z/cos(glm::radians(pitch))));
			yaw = glm::degrees(asin(front.x/cos(glm::radians(pitch))));
		}
		void updatePivotViewMat()
		{
			// -yaw to match obj ro
			position = glm::vec3(glm::rotate(glm::radians(-yaw), glm::vec3(0, 1, 0))
								 * glm::rotate(glm::radians(pitch), glm::vec3(1, 0, 0))
								 * glm::vec4(0, 0, distance, 1));
			printCameraState();
			front = normalize(-position);
			viewMat = glm::lookAt(position, glm::vec3(0), glm::vec3(0, 1, 0));
		}
		void updateFreeViewMat()
		{
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
		void printCameraState()
		{
			Logger::get().log("PYR  : %f.2, %f.2, %f.2\n", pitch, yaw, roll);
			Logger::get().log("POS  : %f.2, %f.2, %f.2\n", position.x, position.y, position.z);
			Logger::get().log("DIST : %f\n", distance);
		}
	private:
		void updateVPMat()
		{
			vpMat = viewMat*projMat;
		}
	};
} // namespace lim
#endif