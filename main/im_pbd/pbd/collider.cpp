/*
    2024-07-31 / imdongye

*/

#include "pbd.h"
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using std::vector;
using namespace pbd;

ColliderPlane::ColliderPlane(const glm::vec3& _p, const glm::vec3& _n)
    : p(_p), n(_n)
{
}
float ColliderPlane::getSD(const glm::vec3& target) const
{
    return dot(n, target - p);
}