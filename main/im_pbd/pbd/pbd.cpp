/*
    2024-07-17 / imdongye

*/
#include "pbd.h"
#include <limbrary/glm_tools.h>

using namespace glm;
using std::vector;

static inline bool isSameEdges( uint a1, uint a2, uint b1, uint b2 ) {
    return ( a1 == b1 && a2 == b2 ) || ( a1 == b2 && a2 == b1 );
}
static bool getSameEdges( uint a1, uint a2, uint b1, uint b2, uvec2& edge ) {
    if( isSameEdges( a1, a2, b1, b2 ) ) {
        edge = (a1<a2)?uvec2{a1,a2}:uvec2{a2,a1};
        return true;
    }
    return false;
}
static bool isAdjacentTris( uvec3 t1, uvec3 t2 ) {
    return  isSameEdges( t1.x, t1.y, t2.x, t2.y ) ||
            isSameEdges( t1.x, t1.y, t2.x, t2.z ) ||
            isSameEdges( t1.x, t1.y, t2.y, t2.z ) ||
            isSameEdges( t1.x, t1.z, t2.x, t2.y ) ||
            isSameEdges( t1.x, t1.z, t2.x, t2.z ) ||
            isSameEdges( t1.x, t1.z, t2.y, t2.x ) ||
            isSameEdges( t1.y, t1.z, t2.x, t2.y ) ||
            isSameEdges( t1.y, t1.z, t2.x, t2.z ) ||
            isSameEdges( t1.y, t1.z, t2.y, t2.z );
}
static int getNrEdges( const vector<uvec3>& tris ) {
    int nr_edges = 0;
    for( const auto& t1 : tris ) {
        for( const auto& t2: tris ) {
            if( &t1 == &t2 ) {
                continue;
            }
            if( isAdjacentTris( t1, t2 ) ) {
                nr_edges++;
            }
        }
    }
    return nr_edges;
}
static bool getAdjacentEdge( uvec3 t1, uvec3 t2,  uvec2& edge ) {
    return  getSameEdges( t1.x, t1.y, t2.x, t2.y, edge ) ||
            getSameEdges( t1.x, t1.y, t2.x, t2.z, edge ) ||
            getSameEdges( t1.x, t1.y, t2.y, t2.z, edge ) ||
            getSameEdges( t1.x, t1.z, t2.x, t2.y, edge ) ||
            getSameEdges( t1.x, t1.z, t2.x, t2.z, edge ) ||
            getSameEdges( t1.x, t1.z, t2.y, t2.x, edge ) ||
            getSameEdges( t1.y, t1.z, t2.x, t2.y, edge ) ||
            getSameEdges( t1.y, t1.z, t2.x, t2.z, edge ) ||
            getSameEdges( t1.y, t1.z, t2.y, t2.z, edge );
}



pbd::SoftMesh::SoftMesh(const lim::Mesh& src) {
    nr_ptcls = src.poss.size();
    x_s.reserve(nr_ptcls);
    p_s.reserve(nr_ptcls);
    v_s.reserve(nr_ptcls);
    for( const auto& p : src.poss ) {
        x_s.push_back( glm::vec4{p,1} );
        p_s.push_back( glm::vec4{p,1} );
        v_s.push_back( glm::vec4{0} );
    }

    nr_tris = src.tris.size();
    tris = src.tris;

    nr_edges = getNrEdges( tris );
    edges.reserve( nr_edges );
    uvec2 edgeIdxs;
    for( int i=0; i < nr_tris; i++ ) {
        for( int j=0; j<nr_tris; j++) {
            if( i==j ) {
                continue;
            }
            const uvec3& t1 = tris[i];
            const uvec3& t2 = tris[j];
            if( getAdjacentEdge( t1, t2, edgeIdxs ) ) {
                float edgeLen  = length( x_s[edgeIdxs.x] - x_s[edgeIdxs.y] );
                vec3 N1 = glim::triNormal( x_s[t1.x], x_s[t1.y], x_s[t1.z] );
                vec3 N2 = glim::triNormal( x_s[t2.x], x_s[t2.y], x_s[t2.z] );
                float cosAngle = dot(N1,N2);
                edges.emplace_back( edgeIdxs, uvec2{i,j}, edgeLen, cosAngle );
            }
        }
    }
    fst_volume = updateVolume();
}

float pbd::SoftMesh::updateVolume() {
    volume = 0;
    for( const auto& t : tris ) {
        volume += glim::triVolume( x_s[t.x], x_s[t.y], x_s[t.z] );
    }
    return volume;
}

namespace {
    constexpr glm::vec3 G = {0, -9.8, 0};
}

static void applyDistanceConstraint( vec4& pm0, vec4& pm1, float fstL, float& lambda, float alpha ) {
    // C(p0, p1) = ||p0 - p1|| - L
    // dC(p0, p1)/dp0 =  (p0 - p1)/||p0 - p1||
    // dC(p0, p1)/dp1 = -(p0 - p1)/||p0 - p1||
    // dL = (-C(p0, p1) - Alpha) / (invM0+invM1 + Alpha)
    float m0 = pm0.w;
    float m1 = pm1.w;
    vec3 p0 = vec3(pm0);
    vec3 p1 = vec3(pm1);
    vec3 diff = p1 - p0;
    float dist = length(diff);
    float Cj = dist - fstL;
    if( glim::isZero( Cj ) ) {
        vec3 dir = {0,1,0};
        float dLambda = (-Cj - alpha*lambda) / (1.f/m0 + 1.f/m1 + alpha);
        pm0 = vec4( p0 + dLambda/m0 * dir, m0 );
        pm1 = vec4( p1 - dLambda/m1 * dir, m0 );
        lambda += dLambda;
    }
}

void pbd::simulate(pbd::SoftMesh& mesh, float dt) {
    constexpr int nr_steps = 10;

    for( int i=0; i<mesh.nr_ptcls; i++ ) {
        const float msss = mesh.x_s[i].w;
        vec4 newVel = mesh.v_s[i] + vec4(G,0);;
        mesh.p_s[i] =  mesh.x_s[i] + newVel*dt;
    }

    for( int i=0; i<mesh.nr_edges; i++ ) {
        auto& e = mesh.edges[i];
        e.lam_length = 0.f;
    }

    float alphaDist = 0.1f/sqrt(dt);

    for( int step=0; step<nr_steps; step++ ) {
        for( int i=0; i<mesh.nr_edges; i++ ) {
            auto& e = mesh.edges[i];
            applyDistanceConstraint( mesh.p_s[e.idx_ps.x], mesh.p_s[e.idx_ps.y], e.fst_length, e.lam_length, alphaDist );
        }

        for( int i=0; i<mesh.nr_ptcls; i++ ) {
            if( mesh.p_s[i].y < 0.001f ) {
                mesh.p_s[i].y = 0.001f;
            }
        }    
    }
    
    for( int i=0; i<mesh.nr_ptcls; i++ ) {
        mesh.v_s[i] = (mesh.p_s[i] - mesh.x_s[i]);
        mesh.x_s[i] = mesh.p_s[i];
    }
}