#include "pbd.h"
#include <algorithm>
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using namespace pbd;
using std::vector;


static inline uvec2 makeEdgeIdx(uint a1, uint a2) {
    return (a1<a2)?uvec2{a1,a2}:uvec2{a2,a1};
}

SoftBody::Compliance::Compliance()
    : stretch(0.001f), shear(0.001f), dist_bend(0.001f)
    , dih_bend(0.001f), iso_bend(0.001f)
    , glo_volume(0.001f)
{
}


SoftBody::SoftBody()
    : total_mass(1.f), inv_ptcl_mass(1.f), nr_ptcls(0), nr_tris(0)
{
    constexpr int defalutSize = 50;
    poss.reserve(defalutSize);
    np_s.reserve(defalutSize);
    v_s.reserve(defalutSize);
    f_s.reserve(defalutSize);
    w_s.reserve(defalutSize);
}
void SoftBody::addPtcl(const vec3& p, float w, const vec3& v) {
    nr_ptcls++;
    poss.push_back( p );
    np_s.push_back( p );
    v_s.push_back( v );
    f_s.push_back( vec3{0} );
    w_s.push_back( w );
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
// ref From: CreateConstraints() in SoftBodySharedcpp of JoltPhysics
SoftBody::SoftBody(const lim::Mesh& src, int nrShear, BendType bendType, float totalMass)
    : lim::Mesh(src), total_mass(totalMass), compliance()
{
    nr_tris = tris.size();
    nr_ptcls = poss.size();
    np_s.reserve(nr_ptcls);
    v_s.reserve(nr_ptcls);
    f_s.reserve(nr_ptcls);
    w_s.reserve(nr_ptcls);

    float ptclMass = total_mass / float(nr_ptcls); // ex) 0.01
    inv_ptcl_mass = 1.f / ptclMass;            // ex) 100.0

    for( const vec3& p : poss ) {
        np_s.push_back( p );
        v_s.push_back( vec3{0} );
        f_s.push_back( vec3{0} );
        w_s.push_back( inv_ptcl_mass );
    }

    struct Edge {
        uvec2 idx_ps;
        uint idx_opp;
        int idx_tri;
    };
    vector<Edge> aEdges; // aside edges

    aEdges.reserve( nr_tris * 3 );
    for( int i=0; i<nr_tris; i++ ) {
        const uvec3& tri = tris[i];
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.y), tri.z, i} );
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.z), tri.y, i} );
        aEdges.push_back( {makeEdgeIdx(tri.y, tri.z), tri.x, i} );
    }
    std::sort(aEdges.begin(), aEdges.end(), [](const Edge& a, const Edge& b) {
        return a.idx_ps.x < b.idx_ps.x || (a.idx_ps.x==b.idx_ps.x && a.idx_ps.y < b.idx_ps.y);
    });

    for( int i=0; i<nr_tris*3; i++ ) {
        const Edge& edge1 = aEdges[i];
        bool isShear = false;
        for( uint j=i+1; j<nr_tris*3; j++ )
        {
            const Edge& edge2 = aEdges[j];

            if( edge1.idx_ps!=edge2.idx_ps ) {
                i = j-1;
                break;
            }
            uvec4 idxPs = {edge1.idx_ps, edge1.idx_opp, edge2.idx_opp};
            vec3 e0 = poss[idxPs.x] - poss[idxPs.y];
            vec3 e1 = poss[idxPs.z] - poss[idxPs.y];
            vec3 e2 = poss[idxPs.z] - poss[idxPs.x];
            vec3 e3 = poss[idxPs.w] - poss[idxPs.x];
            vec3 e4 = poss[idxPs.w] - poss[idxPs.y];
            vec3 n1 = normalize(cross(e1, e0));
            vec3 n2 = normalize(cross(e0, e4));

            // check shear
            if( abs(dot(e1, e4)) < 0.0001f && abs(dot(e2, e3)) < 0.0001f && abs(dot(n1, n2)) > 0.9999f ) {
                isShear = true;
                if( nrShear>1 ) {
                    c_shears.push_back( {*this, makeEdgeIdx(edge1.idx_opp, edge2.idx_opp)} );
                }
            }

            // else is bend
            switch( bendType ) {
            case BendType::None:
                break;
            case BendType::Distance: {
                if( isShear && nrShear>1 ) {
                    break;
                }
                bool isInside = false;
                uvec2 idxEdge = makeEdgeIdx(edge1.idx_opp, edge2.idx_opp);
                for( const auto& c : c_dist_bends ) {
                    if( c.idx_ps == idxEdge ) {
                        isInside = true;
                        break;
                    }
                }
                if( !isInside ) {
                    c_dist_bends.push_back( {*this, makeEdgeIdx(edge1.idx_opp, edge2.idx_opp)} );
                }
                break;
            }
            case BendType::Dihedral:
                c_dih_bends.push_back( {*this, idxPs} );
                break;
            case BendType::Isometric:
                c_iso_bends.push_back( {*this, idxPs} );                
                break;
            }
        }
        if( !isShear ) {
            c_stretchs.push_back( {*this, edge1.idx_ps} );
        }
        else if( nrShear>0 ) {
            c_shears.push_back( {*this, edge1.idx_ps} );
        }
    }
}

float SoftBody::getVolume() const {
    float volume = 0;
    for( const auto& t : tris ) {
        volume += glim::signedTetrahedronVolume( poss[t.x], poss[t.y], poss[t.z] );
    }
    return volume;
}

void SoftBody::applyDeltaP(int idx, float lambda, const vec3& dC) {
    np_s[idx] = np_s[idx] + lambda*w_s[idx]*dC;
}