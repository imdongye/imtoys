#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include "sdf_global.h"
using namespace glm;

// sdWorld from shader
namespace {
    float vmax(vec2 v) {
        return max(v.x, v.y);
    }
    float vmax(vec3 v) {
        return max(max(v.x, v.y), v.z);
    }
    float sgn(float x) {
        return (x<0)?-1:1;
    }
    vec2 sgn(vec2 v) {
        return vec2((v.x<0)?-1:1, (v.y<0)?-1:1);
    }
    float sdSphere( vec3 p ) {
        return length(p) - 0.5;
    }
    float sdBox(vec3 p) {
        vec3 d = abs(p) - vec3(0.5);
        return length(max(d, vec3(0))) + vmax(min(d, vec3(0)));
    }
    float sdPipe(vec3 p) {
        float d = length(p.xz()) - 0.5;
        return max(d, abs(p.y) - 1.f);
    }
    float sdDonut( vec3 p, float r ) {
        vec2 q = vec2(length(p.xz())-r,p.y);
        return length(q);
    }
    float fOpUnionChamfer(float a, float b, float r) {
        return min(min(a, b), (a - r + b)*sqrt(0.5f));
    }
    float fOpUnionRound(float a, float b, float r) {
        vec2 u = max(vec2(r - a,r - b), vec2(0));
        return max(r, min (a, b)) - length(u);
    }
    float fOpIntersectionChamfer(float a, float b, float r) {
        return max(max(a, b), (a + r + b)*sqrt(0.5f));
    }
    float fOpIntersectionRound(float a, float b, float r) {
        vec2 u = max(vec2(r + a,r + b), vec2(0));
        return min(-r, max (a, b)) + length(u);
    }
    float fOpDifferenceChamfer (float a, float b, float r) {
        return fOpIntersectionChamfer(a, -b, r);
    }
    float fOpDifferenceRound (float a, float b, float r) {
        return fOpIntersectionRound(a, -b, r);
    }

    // model space distance
    float getPrimDist(int primType, vec3 mPos) {
        switch(primType)
        {
        case PT_SPHERE:
            return sdSphere(mPos);
        case PT_BOX:
            return sdBox(mPos);
        case PT_PIPE:
            return sdPipe(mPos);
        case PT_DONUT:
            return sdDonut(mPos, 1);
        }
        return 0;
    }
    float getObjDist(int objIdx, vec3 wPos) {
        mat4 transform = transforms[objIdx];
        vec3 mPos = (transform*vec4(wPos,1)).xyz();
        float primDist = getPrimDist(prim_types[objIdx], mPos);
        return primDist * scaling_factors[objIdx];
    }
    float operateDist(int opType, float a, float b, float blendness) {
        switch(opType)
        {
        case OT_ADD_ROUND:
            return fOpUnionRound(a, b, blendness);
        case OT_ADD_EDGE:
            return fOpUnionChamfer(a, b, blendness);
        case OT_SUB_ROUND:
            return fOpDifferenceRound(a, b, blendness);
        case OT_SUB_EDGE:
            return fOpDifferenceChamfer(a, b, blendness);
        case OT_INT_ROUND:
            return fOpIntersectionRound(a, b, blendness);
        case OT_INT_EDGE:
            return fOpIntersectionChamfer(a, b, blendness);
        }
        return 0;
    }
}

float sdWorld(vec3 wPos) {
    float dist = far_distance; 
    dist = wPos.y; // plane

    for( int i=0; i<serialized_objs.size(); i++ ) 
    {
        float blendness = blendnesses[i];
        float objDist = getObjDist(i, wPos);
        float tempDist = operateDist(op_types[i], dist, objDist, blendness);
        dist = tempDist;
    }
    return dist;
}