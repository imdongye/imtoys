/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

// temp
#include <limbrary/limgui.h>

using namespace glm;
using std::vector;
using namespace pbd;

ConstraintPoint::ConstraintPoint(const SoftBody& body, int idxP, const vec3& _point) 
    : idx_p(idxP), point(_point)
{
    ori_dist = length(point - body.x_s[idx_p]);
}
void ConstraintPoint::project(SoftBody& body, float alpha)
{
    vec3 p = body.p_s[idx_p];
    float w = body.w_s[idx_p];

    vec3 diff = point - p;
    float dist = length(diff);
    float C = dist - ori_dist;
    vec3 dC = diff / dist;

    float denom = 1.f+w+alpha; // target point is 1kg mass
    if( denom < glim::feps )
        return;
    float lambda = C / denom;
    body.p_s[idx_p] += lambda*w*dC;
}



/*
    Distance constraint
    C = ||p1 - p0|| - d
*/
ConstraintDistance::ConstraintDistance(const SoftBody& body, const ivec2& idxPs)
    : idx_ps(idxPs)
{
    ori_dist = length(body.x_s[idx_ps.x] - body.x_s[idx_ps.y]);
}
void ConstraintDistance::project(SoftBody& body, float alpha)
{
    vec3 p1 = body.p_s[idx_ps.x]; // gause-seidel iteration
    vec3 p2 = body.p_s[idx_ps.y];
    // vec3 p1 = body.poss[idx_ps.x]; // jacobi iteration
    // vec3 p2 = body.poss[idx_ps.y];
    float w1 = body.w_s[idx_ps.x];
    float w2 = body.w_s[idx_ps.y];

    vec3 diff = p1 - p2;
    float dist = length(diff);
    float C = dist - ori_dist;
    vec3 dC = diff / dist;

    // simplified XPBD
    float denom = w1+w2+alpha;
    if( denom < glim::feps )
        return;
    float lambda = C / denom;
    dPi[0] = -lambda*w1*dC;
    dPi[1] = lambda*w2*dC;
    body.p_s[idx_ps.x] += dPi[0];
    body.p_s[idx_ps.y] += dPi[1];
    dPi[0] = normalize(dPi[0])*0.1f;
    dPi[1] = normalize(dPi[1])*0.1f;
    
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
ConstraintDihedralBend::ConstraintDihedralBend(const SoftBody& body, const ivec4& idxPs)
    : idx_ps(idxPs)
{
    vec3 p0 = body.x_s[idx_ps.x];
    vec3 p1 = body.x_s[idx_ps.y];
    vec3 p2 = body.x_s[idx_ps.z];
    vec3 p3 = body.x_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 n1 = cross(e1, e0); float n1sqLen = glm::length2(n1); 
    vec3 n2 = cross(e0, e4); float n2sqLen = glm::length2(n2);
    float mulSqLen = n1sqLen*n2sqLen;
    // now not ensure cosAngle is in [-1, 1] but acosApprox ensured
    float cosAngle = dot(n1,n2)/sqrt(mulSqLen);
    float sign = glm::sign(dot(cross(n1, n2), e0));
    ori_angle = sign * glim::acosApprox(cosAngle);
}
void ConstraintDihedralBend::project(SoftBody& body, float alpha)
{
    for( int i=0; i<4; i++ ) {
        dPi[i] = vec3(0.f);
    }
    vec3 p0 = body.p_s[idx_ps.x]; float w0 = body.w_s[idx_ps.x];
    vec3 p1 = body.p_s[idx_ps.y]; float w1 = body.w_s[idx_ps.y];
    vec3 p2 = body.p_s[idx_ps.z]; float w2 = body.w_s[idx_ps.z];
    vec3 p3 = body.p_s[idx_ps.w]; float w3 = body.w_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 e2 = p2 - p0;
    vec3 e3 = p3 - p0;
    float e0Len = length(e0);
	LimGui::PlotValAddValue("dih_e0Len", e0Len);
    if( e0Len<1.0e-6f )
        return;

    // From: https://www.cs.ubc.ca/~rbridson/docs/cloth2003.pdf
    // based on "Simulation of Clothing with Folds and Wrinkles" - R. Bridson et al. (Section 4)
    vec3 n1 = cross(e1, e0); float n1sqLen = length2(n1);
    vec3 n2 = cross(e0, e4); float n2sqLen = length2(n2);
    float mulSqLen = n1sqLen*n2sqLen;
    if( mulSqLen<1.0e-24f )
        return;
    // if( abs(0.5-n1sqLen/(n1sqLen+n2sqLen)) > 0.4f )
    //     return;
    float cosAngle = dot(n1,n2)/sqrt(mulSqLen);
    n1 /= n1sqLen;
    n2 /= n2sqLen;
    
    float sign = glm::sign(dot(cross(n1, n2), e0));
    float angle = sign * glim::acosApprox(cosAngle);
    float C = angle - ori_angle;
    
    // Ensure the range to [-PI , PI]
    if (C > glim::pi)
        C -= 2.0f * glim::pi;
    else if (C < -glim::pi)
        C += 2.0f * glim::pi;
    LimGui::PlotValAddValue("dih_c", C);
    // float maxPctC = 0.2f;
    // float maxPctC = glim::pi*max(body.params.inv_stiff_dih_bend, 0.1f);
    // if( abs(C) > maxPctC ) {
    //     return;
    //     // C *=0.01f;
    // }
    // if( C < -glim::pi*maxPctC )
    //     C = -glim::pi*maxPctC;
    // if( C >  glim::pi*maxPctC )
    //     C =  glim::pi*maxPctC;
    vec3 u2 = e0Len*n1;
    vec3 u3 = e0Len*n2;
    // vec3 u0 = ( dot(e1,e0)*n1 + dot(e4,e0)*n2 ) / e0Len;
    // vec3 u1 = - u0 - u2 - u3;
    vec3 u1 = -( dot(e1,e0)*n1 + dot(e4,e0)*n2 ) / e0Len;
    vec3 u0 = - u1 - u2 - u3;

    float denom = w0*length2(u0) + w1*length2(u1) + w2*length2(u2) + w3*length2(u3) + alpha;
	LimGui::PlotValAddValue("dih_denom", denom );
    if( denom < 1.0e-12f )
        return;
    float lambda = -C / denom; 

    dPi[0] = lambda * w0 * u0;
    dPi[1] = lambda * w1 * u1;
    dPi[2] = lambda * w2 * u2;
    dPi[3] = lambda * w3 * u3;

    body.p_s[idx_ps.x] += dPi[0];
    body.p_s[idx_ps.y] += dPi[1];
    body.p_s[idx_ps.z] += dPi[2];
    body.p_s[idx_ps.w] += dPi[3];

    // debuging
    for( int i=0; i<4; i++ ) {
        if( body.w_s[idx_ps[i]] == 0.f )
            dPi[i] = vec3(0.f);
        else 
            dPi[i]  = normalize(dPi[i])*0.1f;
    }
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
From: https://carmencincotti.com/2022-08-29/the-isometric-bending-constraint-of-xpbd/
https://carmencincotti.com/2022-08-29/the-isometric-bending-constraint-of-xpbd/

*/
ConstraintIsometricBend::ConstraintIsometricBend(const SoftBody& body, const ivec4& idxPs)
    : idx_ps(idxPs)
{
    const vec3& p0 = body.x_s[idx_ps.x];
    const vec3& p1 = body.x_s[idx_ps.y];
    const vec3& p2 = body.x_s[idx_ps.z];
    const vec3& p3 = body.x_s[idx_ps.w];
    vec3 e0 = p0 - p1;
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 e2 = p2 - p0;
    vec3 e3 = p3 - p0;
    
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
    const vec3 p_s[4] = {
        body.p_s[idx_ps.x],
        body.p_s[idx_ps.y],
        body.p_s[idx_ps.z],
        body.p_s[idx_ps.w]
    };
    const float w_s[4] = {
        body.w_s[idx_ps.x],
        body.w_s[idx_ps.y],
        body.w_s[idx_ps.z],
        body.w_s[idx_ps.w]
    };

    float qs = 0.f;
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
        qs+= Q[i][j]*dot(p_s[i], p_s[j]);
    }
    float C = 0.5f*qs;
    if( C < glim::feps )
        return;

    
    float denom = alpha;
    for( int i=0; i<4; i++ ) {
        dPi[i] = vec3(0);
        for( int j=0; j<4; j++ ) {
            dPi[i] += Q[i][j] * p_s[j];
        }
        denom += length2(dPi[i]) * w_s[i];
    }
    if( denom < glim::feps )
        return;


    float lambda = -C / denom;
    for( int i=0; i<4; i++ ) {
        dPi[i] = lambda * w_s[i] * dPi[i];
        body.p_s[idx_ps[i]] += dPi[i];
        if( w_s[i] > 0.f )
            dPi[i] = normalize(dPi[i])*0.1f;
    }
}


// From: https://namu.wiki/w/%EC%95%95%EB%A0%A5
void ConstraintVolume::applyImpulse(SoftBody& body, float pressure, float dt)
{
    cur_six_volume = body.getVolumeTimesSix();
    if( pressure<glim::feps ) {
        return;
    }
    
    if( cur_six_volume < glim::feps ) {
        return;
    }

    // vec3 F = pressure*twoAreaAndNor/sixVolume;
    float coeff = pressure*dt/cur_six_volume;

    for( ivec3 tri : body.ptcl_tris ) {
        vec3 e1 = body.p_s[tri.y] - body.p_s[tri.x];
        vec3 e2 = body.p_s[tri.z] - body.p_s[tri.x];
        vec3 twoAreaAndNor = cross(e1, e2);

       body.v_s[tri.x] += body.w_s[tri.x]*coeff * twoAreaAndNor;
       body.v_s[tri.y] += body.w_s[tri.y]*coeff * twoAreaAndNor;
       body.v_s[tri.z] += body.w_s[tri.z]*coeff * twoAreaAndNor;
    }
}

void ConstraintVolume::projectImpulse(SoftBody& body, float pressure, float dt)
{
    if( pressure<glim::feps ) {
        return;
    }
    
    cur_six_volume = body.getVolumeTimesSix();
    if( cur_six_volume < glim::feps ) {
        return;
    }


    // vec3 F = pressure*twoAreaAndNor/sixVolume;
    float coeff = pressure*dt/cur_six_volume;

    for( ivec3 tri : body.ptcl_tris ) {
        vec3 e1 = body.p_s[tri.y] - body.p_s[tri.x];
        vec3 e2 = body.p_s[tri.z] - body.p_s[tri.x];
        vec3 twoAreaAndNor = cross(e1, e2);

       body.p_s[tri.x] += body.w_s[tri.x]*coeff * twoAreaAndNor;
       body.p_s[tri.y] += body.w_s[tri.y]*coeff * twoAreaAndNor;
       body.p_s[tri.z] += body.w_s[tri.z]*coeff * twoAreaAndNor;
    }
}

void ConstraintVolume::projectXpbd(SoftBody& body, float pressure, float alpha)
{
    cur_six_volume = body.getVolumeTimesSix();
    if( pressure<glim::feps ) {
        return;
    }
    
    if( cur_six_volume < glim::feps ) {
        return;
    }

    // use v_s to temp gradient variable
    for( const ivec3& tri : body.ptcl_tris ) {
        const vec3& p1 = body.p_s[tri.x];
        const vec3& p2 = body.p_s[tri.y];
        const vec3& p3 = body.p_s[tri.z];
        body.v_s[tri.x] += cross(p3, p2)/6.f;
        body.v_s[tri.y] += cross(p1, p3)/6.f;
        body.v_s[tri.z] += cross(p2, p1)/6.f;
    }

    float sixC = cur_six_volume - pressure*ori_six_volume;
    if( abs(sixC) < glim::feps )
        return;

    float denom = alpha;
    for( int i=0; i<body.nr_ptcls; i++ ) {
        denom += body.w_s[i]*length2(body.v_s[i]);
    }
    if( denom < glim::feps )
        return;
    float lambda = sixC / (6.f*denom);

    for( int i=0; i<body.nr_ptcls; i++ ) {
        body.p_s[i] = lambda * body.w_s[i] * body.v_s[i];
    }
}