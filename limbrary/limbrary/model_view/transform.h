/*

2023-11-02 / im dong ye

Todo: mat to pos ori scale

*/


#ifndef __transform_h_
#define __transform_h_

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <functional>

namespace lim 
{
    struct Transform
    {
        glm::vec3 pos = glm::vec3(0);
		glm::quat ori = glm::quat(1,0,0,0);
		glm::vec3 scale = glm::vec3(1);

        glm::mat4 mtx = glm::mat4(1);

        std::function<void(const Transform* tf)> update_callback = nullptr;
        void update();
        void decomposeMtx();
    };
    struct TransformPivoted : public Transform
    {
        glm::vec3 pivot = glm::vec3(0);
        float theta, phi; // degree
        float dist = 0;

        glm::vec3 dir; // out rst
        
        void updateWithRotAndDist();
        // void updateOrientation(); // todo
    };

    inline glm::mat4 getMtxTf(const Transform* tf) {
		return (tf) ? tf->mtx : glm::mat4(1.f);
	}
}

#endif