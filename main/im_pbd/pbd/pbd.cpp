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
pbd::SoftBody::SoftBody(const lim::Mesh& src, BendType bendType) {
    nr_ptcls = src.poss.size();
    xw_s.reserve(nr_ptcls);
    pw_s.reserve(nr_ptcls);
    v0_s.reserve(nr_ptcls);
    for( const vec3& p : src.poss ) {
        xw_s.push_back( glm::vec4{p,1} );
        pw_s.push_back( glm::vec4{p,1} );
        v0_s.push_back( glm::vec4{0} );
    }

    nr_tris = src.tris.size();
    tris = src.tris;

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

    for( int i=0; i<nr_tris*3; i++ ) {
        const Edge& e1 = aEdges[i];
        bool isShear = false;
        for( int j=i+1; j<nr_tris*3; j++ ) {
            const Edge& e2 = aEdges[j];
            if( e1.idx_ps!=e2.idx_ps ) {
                i = j-1;
                break;
            }

            // Todo check shear
            // ...

            switch( bendType ) {
            case BendType::None:
                break;
            case BendType::Distance: {
                eBend.push_back( makeEdgeIdx(e1.idx_opp, e2.idx_opp));
            } break;
            case BendType::CosAngle: {
                vec3 N1 = glim::triNormal( xw_s[tris[e1.idx_tri].x], xw_s[tris[e1.idx_tri].y], xw_s[tris[e1.idx_tri].z] );
                vec3 N2 = glim::triNormal( xw_s[tris[e2.idx_tri].x], xw_s[tris[e2.idx_tri].y], xw_s[tris[e2.idx_tri].z] );
                float cosAngle = dot(N1,N2);
                c_bendings.emplace_back( uvec2{e1.idx_tri, e2.idx_tri}, cosAngle );
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
        c_distances.emplace_back( e, length(xw_s[e.x]-xw_s[e.y]) );
    }
}

float pbd::SoftBody::getVolume() {
    float volume = 0;
    for( const auto& t : tris ) {
        volume += glim::signedTetrahedronVolume( xw_s[t.x], xw_s[t.y], xw_s[t.z] );
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
void pbd::ConstraintDistance::project(SoftBody& body, float dt) {
    vec4& pw1 = body.pw_s[idx_ps.x];
    vec3 p1 = vec3(pw1);
    float w1 = pw1.w;
    vec4& pw2 = body.pw_s[idx_ps.y];
    vec3 p2 = vec3(pw2);
    float w2 = pw2.w;

    vec3 diff = p2 - p1;
    float dist = length(diff);
    vec3 dir = diff / dist;

    float s = (dist-ori_dist) / (w1+w2);
    vec3 dP1 =  (s*w1) *dir;
    vec3 dP2 = -(s*w2) *dir;

    pw1 = vec4(p1 + dP1, w1);
    pw2 = vec4(p2 + dP2, w2);
}



/*
    Bending constraint
*/
pbd::ConstraintBending::ConstraintBending(uvec2 idxTs, float cangle)
    : idx_ts(idxTs), ori_cangle(cangle)
{
}
void pbd::ConstraintBending::project(SoftBody& body, float dt) {
    
}



/*
    Volume constraint
*/
pbd::ConstraintVolume::ConstraintVolume(float volume)
    : ori_volume(volume)
{
}
void pbd::ConstraintVolume::project(SoftBody& body, float dt)
{
    
}





void pbd::SoftBody::updateP(float dt) {
    // (5) update v with external force (ex. G)
    for( int i=0; i<nr_ptcls; i++ ) {
        vec3 v = vec3( v0_s[i] );
        v += G*dt;
        v0_s[i] = vec4{v, 0};
    }

    // (6) dampVel
    // ...

    // (7) update p
    for( int i=0; i<nr_ptcls; i++ ) {
        pw_s[i] = xw_s[i] + v0_s[i]*dt;
    }
}

void pbd::SoftBody::updateX(float dt) {

    // (9) solverIterations
    const int nr_steps = 10;
    for( int i=0; i<nr_steps; i++ ) {
        for( auto& c : c_distances ) {
            c.project( *this, dt );
        }
        for( auto& c : c_bendings ) {
            c.project( *this, dt );
        }
        for( auto& c : c_volumes ) {
            c.project( *this, dt );
        }
    }

    // (12) update x, v
    for( int i=0; i<nr_ptcls; i++ ) {
        vec3 v = vec3{pw_s[i]} - vec3{xw_s[i]};
        xw_s[i] = pw_s[i];
        v0_s[i] = vec4{v, 0}/dt;
    }
}




void pbd::Simulator::update(float dt) {
    constexpr int nr_steps = 10;

    for( auto& body : bodies ) {
        body.updateP( dt );
    }
    // (8) generate collision constraints
    for( auto& body : bodies ) {
        for( int i=0; i<body.nr_ptcls; i++ ) {
            vec4& pw = body.pw_s[i];
            vec4& xw = body.xw_s[i];
            if( pw.y<0 ) {
                float temp = pw.y;
                pw.y = xw.y;
                xw.y = temp;
            }
        }
    }

    for( auto& body : bodies ) {
        body.updateX( dt );
    }

    // (16) velocityUpdate
    // velocities of colliding vertices are modified according to 
    // friction and restitution coefficients
}