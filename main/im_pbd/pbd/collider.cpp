/*
    2024-07-31 / imdongye

*/

#include "pbd.h"
#include <limbrary/tools/glim.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using std::vector;
using namespace pbd;


float ColliderPlane::getSdNor( const vec3& p, vec3& outNor ) const
{
    outNor = tf.ori*glim::front;
    float r = dot(tf.pos, outNor);
    return dot(p, outNor) - r;
}

float ColliderSphere::getSdNor( const vec3& p, vec3& outNor ) const
{
    vec3 diff = p - tf.pos;
    float dist = length(diff);
    outNor = diff/dist;
    return dist - tf.scale.x*0.5f;
}

float SoftBody::getSdNor( const vec3& p, vec3& outNor ) const
{
    // not implimented
    return 100;
}