/*

2023-11-02 / im dong ye

parent는 계층관계나 pivot으로 쓴다.

*/


#include <limbrary/model_view/transform.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/orthonormalize.hpp>

using namespace lim;

void Transform::update() {
    mtx = glm::translate(pos) 
        * glm::mat4_cast(ori) 
        * glm::scale(scale);
    
    if( update_callback )
        update_callback(this);
}
void Transform::decomposeMtx() {
    scale = {glm::length(glm::vec3(mtx[0])), glm::length(glm::vec3(mtx[1])), glm::length(glm::vec3(mtx[2]))};
    pos = glm::vec3(mtx[3]);
    glm::mat3 ortho = glm::orthonormalize(glm::mat3(mtx));
    // ori = glm::normalize(glm::quat_cast(glm::mat3(mtx)));
    ori = glm::quat_cast(ortho);
    // glm::vec3 skew;
    // glm::vec4 perspective;
    // glm::decompose(mtx, scale, ori, pos, skew, perspective);
    // ori = glm::conjugate(ori);
}

void TransformPivoted::updateWithRotAndDist() {
    // Todo: make mat
	glm::vec4 toPos = {0,1,0,0};
	toPos = glm::rotate(glm::radians(theta), glm::vec3{0,0,-1}) * toPos;
	toPos = glm::rotate(glm::radians(phi),   glm::vec3{0,1, 0}) * toPos;
    dir = toPos;
	pos = pivot+dist*dir;
    update();
}



void TransformWithInv::update() {
    mtx = glm::translate(pos) 
        * glm::mat4_cast(ori) 
        * glm::scale(scale);
    inv_mtx = glm::inverse(mtx);
    
    if( update_callback )
        update_callback(this);
}