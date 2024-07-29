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
                // eShear.push_back(makeEdgeIdx(edge1.idx_opp, edge2.idx_opp));
                // continue;
            }

            // else is bend
            switch( settings.bendType ) {
            case Settings::BendType::None:
                break;
            case Settings::BendType::Distance: {
                if( isShear )
                    break;
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
    // for( const auto& e : eShear ) {
    //     c_distances.emplace_back( e, length(poss[e.x]-poss[e.y]) );
    // }
    // for( const auto& e : eBend ) {
    //     c_distances.emplace_back( e, length(poss[e.x]-poss[e.y]) );
    // }
    if( settings.apply_volume_constraint ) {
        c_volumes.emplace_back( getVolume(), nr_ptcls );
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
    p1 = p1 - lambda*w1*dC;
    p2 = p2 + lambda*w2*dC;
}


#pragma region Bending-constraint

pbd::ConstraintBending::ConstraintBending(uvec4 idxPs, float angle)
    : idx_ps(idxPs), ori_angle(angle)
{
}
void pbd::ConstraintBending::project(SoftBody& body, float alpha) {
    vec3& p0 = body.np_s[idx_ps.x]; float w0 = body.w_s[idx_ps.x];
    vec3& p1 = body.np_s[idx_ps.y]; float w1 = body.w_s[idx_ps.y];
    vec3& p2 = body.np_s[idx_ps.z]; float w2 = body.w_s[idx_ps.z];
    vec3& p3 = body.np_s[idx_ps.w]; float w3 = body.w_s[idx_ps.w];
    /* PBD, Appendix A: Bending constraint proejection
       p2
    e2/  \e1
     /    \
    p0----p1 
     \ e0 /
    e3\  /e4
       p3
    */
    vec3 e0 = p0 - p1;
    float e0Len = length(e0);
    vec3 e1 = p2 - p1;
    vec3 e4 = p3 - p1;
    vec3 n1 = normalize(cross(e1, e0));
    vec3 n2 = normalize(cross(e0, e4));
    float d = dot(n1, n2);
    float C = acos(d) - ori_angle;

    dCi[0] = -(dot(e1, e0)*n1 + dot(e4, e0)*n2) / e0Len;
    dCi[2] = n1 * e0Len;
    dCi[3] = n2 * e0Len;
    dCi[1] = -dCi[0] - dCi[2] - dCi[3];

    float denom = alpha;
    for( int i=0; i<4; i++ ) {
        denom += body.w_s[idx_ps[i]] * length2(dCi[i]);
    }
    float lambda = C / denom;
    for( int i=0; i<4; i++ ) {
        body.applyDeltaP(idx_ps[i], lambda, dCi[i]);
    }
}





pbd::ConstraintIsometricBending::ConstraintIsometricBending(uvec4& idxPs, mat4& _q)
    : idx_ps(idxPs), Q(_q)
{
}
void pbd::ConstraintIsometricBending::project(SoftBody& body, float alpha)
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



#pragma endregion Bending-constraint



/*
    Volume constraint
*/
pbd::ConstraintVolume::ConstraintVolume(float volume, int nrPtcls)
    : ori_volume(volume), dCi(nrPtcls)
{
}
void pbd::ConstraintVolume::project(SoftBody& body, float alpha)
{
    float C = body.getVolume() - ori_volume;
    if( C == 0.f ) return;

    std::fill(dCi.begin(), dCi.end(), vec3(0));
    
    for( const uvec3& tri : body.tris ) {
        vec3& p1 = body.np_s[tri.x];
        vec3& p2 = body.np_s[tri.y];
        vec3& p3 = body.np_s[tri.z];
        dCi[tri.x] += cross(p2, p3)/6.f;
        dCi[tri.y] += cross(p3, p1)/6.f;
        dCi[tri.z] += cross(p1, p2)/6.f;
    }

    float denom = alpha;
    for( uint i=0; i<body.nr_ptcls; i++ ) {
        denom += body.w_s[i]*length2(dCi[i]);
    }
    if( denom == 0.f ) return;
    float lambda = C / denom;
    for( uint i=0; i<body.nr_ptcls; i++ ) {
        body.applyDeltaP(i, lambda, dCi[i]);
    }
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

    alpha = settings.a_bending/sqdt;
    for( auto& c : c_i_bendings ) {
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

void pbd::SoftBody::applyDeltaP(int idx, float lambda, const vec3& dC) {
    np_s[idx] = np_s[idx] + lambda*w_s[idx]*dC;
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