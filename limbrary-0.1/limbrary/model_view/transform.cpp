/*

2023-11-02 / im dong ye

parent는 계층관계나 pivot으로 쓴다.

*/


#include "transform.h"

using namespace lim;

void Transform::update() {
    mat = glm::translate(position) 
        * glm::toMat4(orientation) 
        * glm::scale(scale);
}

void TransformPivoted::updatePosDir() {
    // Todo: make mat
	glm::vec4 toPos = {0,1,0,0};
	toPos = glm::rotate(glm::radians(rot.y), glm::vec3{0,0,-1}) * toPos;
	toPos = glm::rotate(glm::radians(rot.x), glm::vec3{0,1, 0}) * toPos;
    direction = toPos;
	position = pivot+distance*direction;
}
void TransformPivoted::updateOrientation() {

}   