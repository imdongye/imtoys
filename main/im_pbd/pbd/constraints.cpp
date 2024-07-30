/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using std::vector;
using namespace pbd;

ConstraintFix::ConstraintFix(int _idx, const vec3& _p) 
    : idx(_idx), p(_p)
{
}
void ConstraintFix::project(SoftBody& body) 
{
    body.np_s[idx] = p;
}



/*
    Distance constraint
    C = ||p1 - p0|| - d
*/
ConstraintDistance::ConstraintDistance(const SoftBody& body, const uvec2& idxPs)
    : idx_ps(idxPs)
{
    ori_dist = length(body.np_s[idx_ps.x] - body.np_s[idx_ps.y]);
}
void ConstraintDistance::project(SoftBody& body, float alpha) 
{
    vec3& p1 = body.np_s[idx_ps.x];
    float w1 = body.w_s[idx_ps.x];
    vec3& p2 = body.np_s[idx_ps.y];
    float w2 = body.w_s[idx_ps.y];

    vec3 diff = p1 - p2;
    float dist = length(diff);
    float C = dist - ori_dist;
    if( C < glim::feps ) return;
    vec3 dC = diff / dist;
    // simplified XPBD
    float lambda = C / (w1+w2+alpha);
    p1 -= lambda*w1*dC;
    p2 += lambda*w2*dC;

    // PBD
    // float stiffness = 0.9f;
    // float lambda = stiffness * C / (w1+w2);
    // vec3 dP1 = -lambda*w1*dC;
    // vec3 dP2 = lambda*w2*dC;
    // body.pw_s[idx_ps.x] = {p1+dP1, w1};
    // body.pw_s[idx_ps.y] = {p2+dP2, w2};

    // original XPBD
    // float dLam = (-C - alpha*lambda)/(w1+w2+alpha);
    // vec3 dP1 =  dLam*w1 * dC;
    // vec3 dP2 = -dLam*w2 * dC;
    // lambda += dLam; // need store lambda in constraints
    // p1 = p1+dP1;
    // p2 = p2+dP2;
}


/*
   n1
   p2
e2/  \e1    e0 = p0-p1
 /    \     e1 = p2-p1
p0----p1    e2 = p2-p0
 \ e0 /     e3 = p3-p0
e3\  /e4    e4 = p3-p1
   p3
   n2
*/
// PBD, Appendix A: Bending constraint proejection
ConstraintDihedralBend::ConstraintDihedralBend(const SoftBody& body, const uvec4& idxPs)
    : idx_ps(idxPs)
{
    vec3 p0 = body.np_s[idx_ps.x];
    vec3 p1 = body.np_s[idx_ps.y];
    vec3 p2 = body.np_s[idx_ps.z];
    vec3 p3 = body.np_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 n1 = normalize(cross(e1, e0)); 
    vec3 n2 = normalize(cross(e0, e4));
    ori_angle = acos(dot(n1,n2));
}
void ConstraintDihedralBend::project(SoftBody& body, float alpha) 
{
    vec3 p0 = body.np_s[idx_ps.x];
    vec3 p1 = body.np_s[idx_ps.y];
    vec3 p2 = body.np_s[idx_ps.z];
    vec3 p3 = body.np_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;

    // From: https://www.cs.ubc.ca/~rbridson/docs/cloth2003.pdf
    // based on "Simulation of Clothing with Folds and Wrinkles" - R. Bridson et al. (Section 4)
    vec3 n1scaled = cross(e1, e0);
    vec3 n2scaled = cross(e0, e4);
    float sqLenN1 = dot(n1scaled, n1scaled);
    float sqLenN2 = dot(n2scaled, n2scaled);
    float cosAngle = dot(n1scaled,n2scaled)/sqrt(sqLenN1*sqLenN2);
    float angle = acos(cosAngle);
    float C = angle - ori_angle;
    if( C < glim::feps ) return;
    float dAngle =  -1.f/ sqrt(1.f-cosAngle*cosAngle);
    float e0Len = length(e0);
    n1scaled /= sqLenN1;
    n2scaled /= sqLenN2;
    dCi[2] = e0Len*n1scaled;
    dCi[3] = e0Len*n2scaled;
    dCi[0] = ( dot(e1,e0)*n1scaled + dot(e4,e0)*n2scaled ) / e0Len;
    dCi[1] = - dCi[2] - dCi[3] - dCi[0];

    float denom = 0;
    for( int i=0; i<4; i++ ) {
        denom += body.w_s[idx_ps[i]]*length2(dCi[i]);
    }
    float lambda = -C / denom;

    for( int i=0; i<4; i++ ) {
        body.applyDeltaP(idx_ps[i], lambda, dCi[i]);
        dCi[i] *= lambda*10000000.f;
    }
}





ConstraintIsometricBend::ConstraintIsometricBend(const SoftBody& body, const uvec4& idxPs)
    : idx_ps(idxPs)
{
    const vec3& p0 = body.np_s[idx_ps.x];
    const vec3& p1 = body.np_s[idx_ps.y];
    const vec3& p2 = body.np_s[idx_ps.z];
    const vec3& p3 = body.np_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e2 = p2 - p0;
    vec3 e3 = p3 - p0;
    vec3 e4 = p3 - p1;
    // Q = 3/area * Kt*K
    // K = { c01+c04, c02+c03, -c01-c02, -c03-c04 }
    // cjk = cot(ej,ek)
    float c01 = glim::cotInterVec( e1, e0);
    float c02 = glim::cotInterVec(-e0, e2);
    float c03 = glim::cotInterVec( e3,-e0);
    float c04 = glim::cotInterVec( e0, e4);
    vec4 K = {
         c01+c04,
         c02+c03,
        -c01-c02,
        -c03-c04
    };
    float area = 0.5f*(length(cross(e1,e0)) + length(cross(e0,e4)));
    Q = 3.f/area * outerProduct(K, K);
}
void ConstraintIsometricBend::project(SoftBody& body, float alpha)
{
    float qs = 0.f;
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
        qs+= Q[i][j]*dot(body.np_s[i], body.np_s[j]);
    }
    float C = 0.5f*qs;
    if( C == 0.f ) return;
    float denom = alpha;
    for( int i=0; i<4; i++ ) {
        dCi[i] = vec3(0);
        for( int j=0; j<4; j++ ) {
            dCi[i] += Q[j][i] * body.np_s[idx_ps[j]];
        }
        denom += length2(dCi[i]) * body.w_s[idx_ps[i]];
    }
    if( denom == 0.f ) return;


    float lambda = -C / denom;
    for( int i=0; i<4; i++ ) {
        body.applyDeltaP(idx_ps[i], lambda, dCi[i]);
    }
}





/*
    Volume constraint
*/
ConstraintGlobalVolume::ConstraintGlobalVolume(const SoftBody& body)
    : ori_volume(body.getVolume()), dCi(body.nr_ptcls)
{
}
void ConstraintGlobalVolume::project(SoftBody& body, float alpha)
{

    std::fill(dCi.begin(), dCi.end(), vec3(0));
    
    for( const uvec3& tri : body.tris ) {
        vec3& p0 = body.np_s[tri.x];
        vec3& p1 = body.np_s[tri.y];
        vec3& p2 = body.np_s[tri.z];
        dCi[tri.x] += cross(p1, p2)/6.f;
        dCi[tri.y] += cross(p2, p0)/6.f;
        dCi[tri.z] += cross(p0, p1)/6.f;
    }

    float C = body.getVolume() - ori_volume;
    float denom = alpha;
    for( int i=0; i<body.nr_ptcls; i++ ) {
        denom += body.w_s[i]*dot(dCi[i], dCi[i]);
    }
    float lambda = C / denom;

    for( int i=0; i<body.nr_ptcls; i++ ) {
        body.applyDeltaP(i, lambda, dCi[i]);
    }
}
