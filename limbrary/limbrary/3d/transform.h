/*
    imdongye@naver.com
	fst: 2023-11-02
	lst: 2023-11-02
*/


#ifndef __transform_h_
#define __transform_h_

#include <glm/glm.hpp>
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
        // inline Transform operator*(const Transform& tf) {
        //     Transform rst;
        //     rst.mtx = mtx*tf.mtx;
        //     return rst;
        // }
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

    struct TransformWithInv : public Transform
    {
        glm::mat4 inv_mtx = glm::mat4(1);

        void update();
    };

    inline glm::mat4 getMtxTf(const Transform* tf) {
		return (tf==nullptr) ? glm::mat4(1.f) : tf->mtx;
	}
}

#endif