/*

2023-11-02 / im dong ye

parent는 계층관계나 pivot으로 쓴다.

*/


#ifndef __transform_h_
#define __transform_h_

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lim 
{
    class Transform
    {
    public:
        glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat(1,0,0,0);
		glm::vec3 scale = glm::vec3(1);
        // translateMat * rotateMat * scaleMat
        glm::mat4 transform = glm::mat4(1);
    public:
        void updateTransform() {
            glm::mat4 translateMat = glm::translate(position);
            glm::mat4 scaleMat = glm::scale(scale);
            glm::mat4 rotateMat = glm::toMat4(orientation);
            transform = translateMat * rotateMat * scaleMat;
        }
        Transform() = default;
        Transform(const Transform& copy) = default;
        Transform(Transform&& move) = default;
        Transform& operator=(const Transform& copy) = default;
        Transform& operator=(Transform&& move) = default;
    };

    class TransformWithInv
    {
    public:
        glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat(1,0,0,0);
		glm::vec3 scale = glm::vec3(1);
        // translateMat * rotateMat * scaleMat
        glm::mat4 transform = glm::mat4(1);
        glm::mat4 transform_inv = glm::mat4(1);
    public:
        void updateTransform() {
            glm::mat4 translateMat = glm::translate(position);
            glm::mat4 scaleMat = glm::scale(scale);
            glm::mat4 rotateMat = glm::toMat4(orientation);
            transform = translateMat * rotateMat * scaleMat;
            transform_inv = glm::inverse(transform);
        }
        TransformWithInv() = default;
        TransformWithInv(const TransformWithInv& copy) = default;
        TransformWithInv(TransformWithInv&& move) = default;
        TransformWithInv& operator=(const TransformWithInv& copy) = default;
        TransformWithInv& operator=(TransformWithInv&& move) = default;
    };
}

#endif