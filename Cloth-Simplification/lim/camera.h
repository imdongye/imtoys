//
// 2022-07-21 / im dongye
// edit learnopengl code
//
// when controll with function => auto mat update
// when edit prop => must menual update 
//


#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

const float YAW         = -90.f;   // Y
const float PITCH       =  0.f;    // X
const float ROLL        =  0.f;    // Z
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float FOVY        =  45.f;  // ZOOM
const float MAX_FOVY    =  60.f;
const float MIN_FOVY    =  2.f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles,
// Vectors and Matrices for use in OpenGL
struct Camera {
public:
    // camera options
    float movementSpeed;
    float mouseSensitivity;
    float aspect;
    float zNear;
    float zFar;
    // editable
    glm::vec3 pos;
    float yaw;
    float pitch;
    float roll;
    float fovy; // feild of view y axis dir
    // result of inside update
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::mat4 viewMat;
    glm::mat4 projMat;

private:
    void updateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
        // normalize the vectors, because their length gets closer to 0 the more you look up 
        // or down which resu in slower movement.
        right = glm::normalize(glm::cross(front, glm::vec3(0,1,0))); 
        up    = glm::normalize(glm::cross(right, front));
    }
public:
    Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), float _yaw=YAW, float _pitch=PITCH, float _aspect=1.0f)
        : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(SPEED), mouseSensitivity(SENSITIVITY), fovy(FOVY)
        , roll(ROLL), zNear(0.1f), zFar(100.f)
    {
        pos = _position;
        yaw = _yaw;
        pitch = _pitch;
        roll = ROLL;
        aspect = _aspect;
        updateViewMat();
        updateProjMat();
    }
    void updateViewMat() {
        updateCameraVectors();
        viewMat = glm::lookAt(pos, pos + front, glm::vec3(0,1,0)); // todo: roll => edit up
    }
    void updateProjMat(const float aspect=1.0f, const float zNear=0.1f, const float zFar=100.f) {
        projMat = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
    }
    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        switch(direction) {
        case FORWARD:   pos += front * velocity; break;
        case BACKWARD:  pos -= front * velocity; break;
        case LEFT:      pos -= right * velocity; break;
        case RIGHT:     pos += right * velocity; break;
        case UP:        pos += glm::vec3(0,1,0) * velocity; break;
        case DOWN:      pos -= glm::vec3(0,1,0) * velocity; break;
        }
        updateViewMat();
    }
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw   += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }
        updateViewMat();
    }
    void ProcessMouseScroll(float yoffset)
    {
        fovy -= (float)yoffset; // todo: fow graph
        if (fovy < MIN_FOVY)
            fovy = MIN_FOVY;
        if (fovy > MAX_FOVY)
            fovy = MAX_FOVY; 
        updateProjMat();
    }
};
#endif