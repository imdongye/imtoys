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
        glm::mat4 mat = glm::mat4(1);
    public:
        void updateMat() {
            glm::mat4 translateMat = glm::translate(position);
            glm::mat4 scaleMat = glm::scale(scale);
            glm::mat4 rotateMat = glm::toMat4(orientation);
            mat = translateMat * rotateMat * scaleMat;
        }
    };

    class TransformWithInv
    {
    public:
        glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat(1,0,0,0);
		glm::vec3 scale = glm::vec3(1);
        // translateMat * rotateMat * scaleMat * normalize(parent)
        glm::mat4 mat = glm::mat4(1);
        glm::mat4 mat_inv = glm::mat4(1);
    public:
        void updateMat() {
            glm::mat4 translateMat = glm::translate(position);
            glm::mat4 scaleMat = glm::scale(scale);
            glm::mat4 rotateMat = glm::toMat4(orientation);
            mat = translateMat * rotateMat * scaleMat;
            mat_inv = glm::inverse(mat);
        }
    };
}

#endif