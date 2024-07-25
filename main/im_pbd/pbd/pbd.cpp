/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>

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
        const Edge& e1 = aEdges[i];
        bool isShear = false;
        for( uint j=i+1; j<nr_tris*3; j++ )
        {
            const Edge& e2 = aEdges[j];

            if( e1.idx_ps!=e2.idx_ps ) {
                i = j-1;
                break;
            }

            // Todo check shear
            // ...

            switch( settings.bendType ) {
            case Settings::BendType::None:
                break;
            case Settings::BendType::Distance: {
                eBend.push_back( makeEdgeIdx(e1.idx_opp, e2.idx_opp));
            } break;
            case Settings::BendType::CosAngle: {
                /*
                   x2
                e1/  \e3
                 /    \
                x0----x1
                 \ e0 /
                e2\  /e4
                   x3
                */
                uvec4 idxPs = {e1.idx_ps, e1.idx_opp, e2.idx_opp};
                vec3 e0 = poss[idxPs.x] - poss[idxPs.y];
                vec3 e3 = poss[idxPs.z] - poss[idxPs.y];
                vec3 e4 = poss[idxPs.w] - poss[idxPs.y];
                vec3 N1 = cross(e3, e0);
                vec3 N2 = cross(e0, e4);
                float cosAngle = dot(N1,N2);
                c_bendings.emplace_back( idxPs, cosAngle );
            } break;
            }
        }
        if( isShear ) {
            eShear.push_back( e1.idx_ps );
        } else {
            eStretch.push_back( e1.idx_ps );
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




/*
    Distance constraint
    C = ||p1 - p0|| - d
*/
pbd::ConstraintDistance::ConstraintDistance(uvec2 idxPs, float distance)
    : idx_ps(idxPs), ori_dist(distance)
{
}
void pbd::ConstraintDistance::project(SoftBody& body, float alpha) {
    vec3 p1 = body.np_s[idx_ps.x];
    float w1 = body.w_s[idx_ps.x];
    vec3 p2 = body.np_s[idx_ps.y];
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

    // XPBD
    // float dLam = (-C - alpha*lambda)/(w1+w2+alpha);
    // vec3 dP1 =  dLam*w1 * dC;
    // vec3 dP2 = -dLam*w2 * dC;
    
    // lambda -= dLam;
    // body.pw_s[idx_ps.x] = {p1+dP1, w1};
    // body.pw_s[idx_ps.y] = {p2+dP2, w2};


    float lambda = C / (w1+w2+alpha);
    vec3 dP1 = -lambda*w1*dC;
    vec3 dP2 = lambda*w2*dC;
    body.np_s[idx_ps.x] = p1+dP1;
    body.np_s[idx_ps.y] = p2+dP2;
}



/*
    Bending constraint
*/
pbd::ConstraintBending::ConstraintBending(uvec4 idxPs, float cangle)
    : idx_ps(idxPs), ori_cangle(cangle)
{
}
void pbd::ConstraintBending::project(SoftBody& body, float alpha) {
    vec3 e0 = body.np_s[idx_ps.x] - body.np_s[idx_ps.y];
    vec3 e3 = body.np_s[idx_ps.z] - body.np_s[idx_ps.y];
    vec3 e4 = body.np_s[idx_ps.w] - body.np_s[idx_ps.y];
    vec3 N1 = cross(e3, e0);
    vec3 N2 = cross(e0, e4);
    float cosAngle = dot(N1,N2);

    float C = cosAngle - ori_cangle;
    // float lambda = C / (body.getW(idx_ts.x) + body.getW(idx_ts.y) + alpha);
}



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





void pbd::SoftBody::updateP(float dt)
{
    // (5) update v with external force (ex. G)
    for( uint i=0; i<nr_ptcls; i++ ) {
        vec3 v = vec3( v_s[i] );
        v += G*dt;
        v_s[i] = vec4{v, 0};
    }

    // (6) dampVel
    // ...

    // (7) update p
    for( uint i=0; i<nr_ptcls; i++ ) {
        np_s[i] = poss[i] + v_s[i]*dt;
    }
}

void pbd::SoftBody::updateX(float dt)
{
    // (9) solverIterations
    const uint nr_steps = 10;
    float substepDt = dt/nr_steps;
    float sqdt = substepDt*substepDt;
    float alpha;
    for( uint i=0; i<nr_steps; i++ ) {
        alpha = settings.a_distance/sqdt;
        // alpha = 0.f;
        for( auto& c : c_distances ) {
            c.project( *this, alpha );
        }
        alpha = settings.a_bending/sqdt;
        for( auto& c : c_bendings ) {
            c.project( *this, alpha );
        }
        alpha = settings.a_volume/sqdt;
        for( auto& c : c_volumes ) {
            c.project( *this, alpha );
        }
    }

    // (12) update x, v
    for( int i=0; i<nr_ptcls; i++ ) {
        v_s[i] = (np_s[i] - poss[i])/dt;
        poss[i] = np_s[i];
    }
    restorePosBuf();
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