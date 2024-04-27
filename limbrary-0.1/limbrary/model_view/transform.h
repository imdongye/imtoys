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
    struct Transform
    {
        glm::vec3 position = glm::vec3(0);
		glm::quat orientation = glm::quat(1,0,0,0);
		glm::vec3 scale = glm::vec3(1);
        glm::mat4 mat = glm::mat4(1);
        
        void update();
    };
    struct TransformPivoted : public Transform
    {
        glm::vec3 pivot = glm::vec3(0);
        glm::vec2 rot = glm::vec2(0); // degree
        float distance = 0;
        glm::vec3 direction;
        void updatePosDir();
        void updateOrientation();
    };
}

#endif