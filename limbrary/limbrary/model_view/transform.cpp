/*

2023-11-02 / im dong ye

parent는 계층관계나 pivot으로 쓴다.

*/


#include "transform.h"

using namespace lim;

void Transform::update() {
    mtx = glm::translate(pos) 
        * glm::toMat4(orientation) 
        * glm::scale(scale);
    
    if( update_callback )
        update_callback(this);
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