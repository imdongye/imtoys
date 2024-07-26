/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using std::vector;

namespace {
    constexpr glm::vec3 G = {0, -9.8, 0};
}


#pragma region edge_util


static inline uvec2 makeEdgeIdx(uint a1, uint a2) {
    return (a1<a2)?uvec2{a1,a2}:uvec2{a2,a1};
}


#pragma endregion edge_util

// ref From: CreateConstraints() in SoftBodySharedSettings.cpp of JoltPhysics
pbd::SoftBody::SoftBody(const lim::Mesh& src, Settings s)
    : lim::Mesh(src), settings(s)
{
    nr_tris = tris.size();
    nr_ptcls = poss.size();
    np_s.reserve(nr_ptcls);
    v_s.reserve(nr_ptcls);
    w_s.reserve(nr_ptcls);

    for( const vec3& p : poss ) {
        np_s.push_back( p );
        v_s.push_back( vec3{0} );
        w_s.push_back( 1.f );
    }

    struct Edge {
        uvec2 idx_ps;
        uint idx_opp;
        uint idx_tri;
    };
    vector<Edge> aEdges; // aside edges
    vector<uvec2> eStretch;
    vector<uvec2> eShear;
    vector<uvec2> eBend;

    aEdges.reserve( nr_tris * 3 );
    for( uint i=0; i<nr_tris; i++ ) {
        const uvec3& tri = tris[i];
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.y), tri.z, i} );
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.z), tri.y, i} );
        aEdges.push_back( {makeEdgeIdx(tri.y, tri.z), tri.x, i} );
    }
    std::sort(aEdges.begin(), aEdges.end(), [](const Edge& a, const Edge& b) {
        return a.idx_ps.x < b.idx_ps.x || (a.idx_ps.x==b.idx_ps.x && a.idx_ps.y < b.idx_ps.y);
    });

    for( uint i=0; i<nr_tris*3; i++ ) {
        const Edge& edge1 = aEdges[i];
        bool isShear = false;
        for( uint j=i+1; j<nr_tris*3; j++ )
        {
            const Edge& edge2 = aEdges[j];

            if( edge1.idx_ps!=edge2.idx_ps ) {
                i = j-1;
                break;
            }
            /*
               p2
            e2/  \e1
             /    \
            p0----p1 // <-- 주의: 순서 바뀔수있음, 넌리니어한 cos angle의 차이로 계산하면 오차가 더 심해짐
             \ e0 /
            e3\  /e4
               p3
            e0 = p0-p1
            e1 = p2-p1
            e4 = p3-p1
            e2 = p2-p0
            e3 = p3-p0
            */
            uvec4 idxPs = {edge1.idx_ps, edge1.idx_opp, edge2.idx_opp};
            vec3 e0 = poss[idxPs.x] - poss[idxPs.y];
            vec3 e1 = poss[idxPs.z] - poss[idxPs.y];
            vec3 e4 = poss[idxPs.w] - poss[idxPs.y];
            vec3 n1 = normalize(cross(e1, e0));
            vec3 n2 = normalize(cross(e0, e4));

            // check shear
            if( abs(dot(e1, e4)) < 0.0001f && abs(dot(n1, n2)) > 0.9999f ) {
                isShear = true;
                eShear.push_back(makeEdgeIdx(edge1.idx_opp, edge2.idx_opp));
                continue;
            }

            // else is bend
            switch( settings.bendType ) {
            case Settings::BendType::None:
                break;
            case Settings::BendType::Distance: {
                eBend.push_back(makeEdgeIdx(edge1.idx_opp, edge2.idx_opp));
            } break;
            case Settings::BendType::CosAngle: {
                c_bendings.emplace_back( idxPs, acos(dot(n1,n2)) );
            } break;
            case Settings::BendType::Isometric: {
                vec3 e2 = poss[idxPs.z] - poss[idxPs.x];
                vec3 e3 = poss[idxPs.w] - poss[idxPs.x];
                // Q = 3/area * Kt*K
                // K = { c01+c04, c02+c03, -c01-c02, -c03-c04 }
                // cjk = cot(ej,ek)
                vec4 K = {
                     glim::cotInterVec( e1, e0) + glim::cotInterVec( e0, e4),
                     glim::cotInterVec(-e0, e2) + glim::cotInterVec( e3,-e0),
                    -glim::cotInterVec( e1, e0) - glim::cotInterVec(-e0, e2),
                    -glim::cotInterVec( e3,-e0) - glim::cotInterVec( e0, e4),
                };
                float area = 0.5f*(length(cross(e1,e0)) + length(cross(e0,e4)));
                mat4 Q = 3.f/area * outerProduct(K, K);
                c_i_bendings.emplace_back( idxPs, Q );
            } break;
            }
        }
        if( isShear ) {
            eShear.push_back( edge1.idx_ps );
        } else {
            eStretch.push_back( edge1.idx_ps );
        }
    }

    for( const auto& e : eStretch ) {
        c_distances.emplace_back( e, length(poss[e.x]-poss[e.y]) );
    }
    for( const auto& e : eShear ) {
        c_distances.emplace_back( e, length(poss[e.x]-poss[e.y]) );
    }
    for( const auto& e : eBend ) {
        c_distances.emplace_back( e, length(poss[e.x]-poss[e.y]) );
    }
}

float pbd::SoftBody::getVolume() {
    float volume = 0;
    for( const auto& t : tris ) {
        volume += glim::signedTetrahedronVolume( poss[t.x], poss[t.y], poss[t.z] );
    }
    return volume;
}



void pbd::SoftBody::updateP(float dt)
{
    // (5) update v with external force (ex. G)
    for( uint i=0; i<nr_ptcls; i++ ) {
        if( w_s[i]==0.f ) {
            v_s[i] = vec3(0);
            continue;
        }
        v_s[i] += G*dt;
    }

    // (6) dampVel
    // ...

    // (7) update p
    for( uint i=0; i<nr_ptcls; i++ ) {
        np_s[i] = poss[i] + v_s[i]*dt;
    }
}

/*
    Distance constraint
    C = ||p1 - p0|| - d
*/
pbd::ConstraintDistance::ConstraintDistance(uvec2 idxPs, float distance)
    : idx_ps(idxPs), ori_dist(distance)
{
}
void pbd::ConstraintDistance::project(SoftBody& body, float alpha) {
    vec3& p1 = body.np_s[idx_ps.x];
    float w1 = body.w_s[idx_ps.x];
    vec3& p2 = body.np_s[idx_ps.y];
    float w2 = body.w_s[idx_ps.y];

    vec3 diff = p1 - p2;
    float dist = length(diff);
    float C = dist - ori_dist;
    vec3 dC = diff / dist;

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

    // simplified XPBD
    float lambda = C / (w1+w2+alpha);
    vec3 dP1 = -lambda*w1*dC;
    vec3 dP2 = lambda*w2*dC;
    p1 = p1+dP1;
    p2 = p2+dP2;
}


#pragma region Bending-constraint

pbd::ConstraintBending::ConstraintBending(uvec4 idxPs, float angle)
    : idx_ps(idxPs), ori_angle(angle)
{
}
void pbd::ConstraintBending::project(SoftBody& body, float alpha) {
    vec3& p1 = body.np_s[idx_ps.x]; float w1 = body.w_s[idx_ps.x];
    vec3& p2 = body.np_s[idx_ps.y]; float w2 = body.w_s[idx_ps.y];
    vec3& p3 = body.np_s[idx_ps.z]; float w3 = body.w_s[idx_ps.z];
    vec3& p4 = body.np_s[idx_ps.w]; float w4 = body.w_s[idx_ps.w];
    /* PBD, Appendix A: Bending constraint proejection
       p2
    e2/  \e1
     /    \
    p0----p1 
     \ e0 /
    e3\  /e4
       p3
    */
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 e5 = p4 - p1;
    vec3 n1 = normalize(cross(e4, e1)); 
    vec3 n2 = normalize(cross(e1, e5));
    float d = dot(n1,n2);
    float acosD = acos(d);

    // PBD
    vec3 q3 = ( cross(n2,p2) + cross(p2,n1)*d ) / length(cross(p3, p2));
    vec3 q4 = ( cross(n1,p2) + cross(p2,n2)*d ) / length(cross(p4, p2));
    vec3 q2 = ( cross(n2,p3) + cross(p3,n1)*d ) / length(cross(p3, p2))
            - ( cross(n1,p4) + cross(p4,n2)*d ) / length(cross(p4, p2));
    vec3 q1 = -q2-q3-q4;
    float denum = w1*length2(q1) + w2*length2(q2) + w3*length2(q3) + w4*length2(q4) + alpha;
    vec3 dP1 = w1*sqrt(1-d*d)*(acosD-ori_angle)/denum * q1;
    vec3 dP2 = w2*sqrt(1-d*d)*(acosD-ori_angle)/denum * q2;
    vec3 dP3 = w3*sqrt(1-d*d)*(acosD-ori_angle)/denum * q3;
    vec3 dP4 = w4*sqrt(1-d*d)*(acosD-ori_angle)/denum * q4;
    p1 = p1+dP1;
    p2 = p2+dP2;
    p3 = p3+dP3;
    p4 = p4+dP4;
}





pbd::ConstraintIsometricBending::ConstraintIsometricBending(uvec4& idxPs, mat4& _q)
    : idx_ps(idxPs), Q(_q)
{
}
void pbd::ConstraintIsometricBending::project(SoftBody& body, float alpha)
{

}



#pragma endregion Bending-constraint



/*
    Volume constraint
*/
pbd::ConstraintVolume::ConstraintVolume(float volume)
    : ori_volume(volume)
{
}
void pbd::ConstraintVolume::project(SoftBody& body, float alpha)
{
    
}



void pbd::SoftBody::updateX(float dt)
{
    // (9) solverIterations
    float sqdt = dt*dt;
    float alpha;

    alpha = settings.a_volume/sqdt;
    for( auto& c : c_volumes ) {
        c.project( *this, alpha );
    }

    alpha = settings.a_bending/sqdt;
    for( auto& c : c_bendings ) {
        c.project( *this, alpha );
    }
    
    alpha = settings.a_distance/(200.f*sqdt);
    for( auto& c : c_distances ) {
        c.project( *this, alpha );
    }

    // (12) update x, v
    for( uint i=0; i<nr_ptcls; i++ ) {
        v_s[i] = (np_s[i] - poss[i])/dt;
        poss[i] = np_s[i];
    }
   
}



pbd::Simulator::~Simulator()
{
    clear();
}
void pbd::Simulator::clear() {
    for( auto body : bodies ) {
        delete body;
    }
    bodies.clear();
}
void pbd::Simulator::update(float dt) 
{
    const uint nr_steps = 20;
    dt = dt/nr_steps;

    for( uint i=0; i<nr_steps; i++ )
    {
        for( auto body : bodies ) {
            body->updateP( dt );
        }

        // (8) generate collision constraints
        for( auto body : bodies ) {
            for( uint i=0; i<body->nr_ptcls; i++ ) {
                if( body->np_s[i].y<0 ) {
                    // float temp = pw.y;
                    // pw.y = xw.y;
                    // xw.y = temp;
                    body->np_s[i].y = 0;
                }
            }
        }

        for( auto body : bodies ) {
            body->updateX( dt );
        }
        // (16) velocityUpdate
        // velocities of colliding vertices are modified according to 
        // friction and restitution coefficients
    }


    for( auto body : bodies ) {
        if( body->settings.update_buf ) {
            body->restorePosBuf();
        }
    }
}