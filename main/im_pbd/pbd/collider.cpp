/*
    2024-07-31 / imdongye

*/

#include "pbd.h"
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using std::vector;
using namespace pbd;

ColliderPlane::ColliderPlane(const vec3& _n, float r)
    : n(_n), r(r)
{
}
float ColliderPlane::getSdNor( const vec3& p, vec3& outNor ) const
{
    outNor = n;
    return dot(p, n) - r;
}

ColliderSphere::ColliderSphere(const vec3& _c, float r)
    : c(_c), r(r)
{
}
float ColliderSphere::getSdNor( const vec3& p, vec3& outNor ) const
{
    vec3 diff = p - c;
    float dist = length(diff);
    outNor = diff/dist;
    return dist - r;
}

float SoftBody::getSdNor( const vec3& p, vec3& outNor ) const
{
    // not implimented
    return 100;
}