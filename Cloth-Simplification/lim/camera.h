//
//  2022-07-21 / im dongye
//  edit learnopengl code
//
//  when controll with function => auto mat update
//  when edit prop => must menual update
//  if distance < 0 => free view
//  else center pivot view
//
//  TODO list :
//  1. roll
//  2. add centric rotate mode
//
//

#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
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
const float MAX_FOVY = 170.f;
const float MIN_FOVY = 10.f;
const float MAX_DIST = 17.f;
const float MIN_DIST = 2.f;
class Camera {
private:
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
    float distance;
    // result of inside update
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
public:
    glm::mat4 viewMat;
    glm::mat4 projMat;
    glm::mat4 vpMat;
public:
    Camera(float _aspect=1.0f, glm::vec3 _position = glm::vec3(0), float _yaw=0, float _pitch=0)
        : movementSpeed(2.5), mouseSensitivity(0.1), fovy(60), roll(0), zNear(0.1), zFar(100.f)
    {
        pos = _position;
        yaw = _yaw;
        pitch = _pitch;
        aspect = _aspect;
        distance = -1;
        updateFreeViewMat();
        updateProjMat();
    }
    Camera(float _aspect=1.0f, float _distance = 5.0f, float _yaw=0, float _pitch=0)
        : movementSpeed(2.5), mouseSensitivity(0.1), fovy(60), roll(0), zNear(0.1), zFar(100.f)
    {
        distance = _distance;
        yaw = _yaw;
        pitch = _pitch;
        aspect = _aspect;
        updatePivotPos();
        updateProjMat();
    }
    void processMoveInput(Camera_Movement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        
        switch(direction) {
        case FORWARD:   pos += front * velocity; break;
        case BACKWARD:  pos -= front * velocity; break;
        case LEFT:      pos -= right * velocity; break;
        case RIGHT:     pos += right * velocity; break;
        case UP:        pos += glm::vec3(0,1,0) * velocity; break;
        case DOWN:      pos -= glm::vec3(0,1,0) * velocity; break;
        }
        updateFreeViewMat();
    }
    void processKey(int key, int action) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        }
    }
    void processMouse(float xoff, float yoff, int btn, int w, int h, bool constrainPitch = true) {
        xoff *= mouseSensitivity;
        yoff *= mouseSensitivity;
        
        if(btn==3) {
            updatePivotScroll(xoff, yoff, w, h);
            return;
        }
        
        yaw   += xoff;//todo clamp
        pitch += yoff;

        if (constrainPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }
        if(btn==1) {
            updateFreeViewMat();
        }
        else if(btn==2) {
            updatePivotPos();
        }
    }
    void processMouseScroll(float yoffset)
    {
        fovy = fovy * pow(1.03, -yoffset);
        fovy = glm::clamp(fovy, MIN_FOVY, MAX_FOVY);
        updateProjMat();
    }
    void setAspect(float _aspect) {
        aspect = _aspect;
        updateProjMat();
    }
private:
    void updatePivotScroll(float xoff, float yoff, int w, int h) {
        fovy = fovy * pow(1.01, yoff/h * 160.f);
        fovy = glm::clamp(fovy, MIN_FOVY, MAX_FOVY);
        distance *= pow(1.01, xoff/w * 170.f);
        distance = glm::clamp(distance, MIN_DIST, MAX_DIST);
        updateProjMat();
        updatePivotPos();
    }
    void updatePivotPos() {
        pos = glm::vec3( glm::rotate(yaw, glm::vec3(0,1,0))
                         * glm::rotate(pitch, glm::vec3(1,0,0))
                         * glm::vec4(0,0,distance,1) );
        updatePivotViewMat();
    }
    void updatePivotViewMat() {
        viewMat = glm::lookAt(pos, glm::vec3(0), glm::vec3(0,1,0));
        updateVPMat();
    }
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
    void updateVPMat() {
        vpMat = viewMat*projMat;
    }
    // menual update
    void updateFreeViewMat() {
        updateCameraVectors();
        viewMat = glm::lookAt(pos, pos + front, glm::vec3(0,1,0)); // todo: roll => edit up
        updateVPMat();
    }
    void updateProjMat(const float aspect=1.0f, const float zNear=0.1f, const float zFar=100.f) {
        projMat = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);
        updateVPMat();
    }
};
#endif
