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
{
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
SoftBody::SoftBody(const lim::Mesh& src, int nrShear, BendType bendType, float bodyMass)
    : lim::Mesh(src), compliance()
{
    // rm same pos verts to ptcls =============
    vector<int> vertIdxToPtclIdx(nr_verts, -1);
    vector<vec3>  tempPtcls;
    vector<vector<int>> ptclIdxToVertIdxs;
    tempPtcls.reserve(nr_verts);
    ptclIdxToVertIdxs.reserve(nr_verts);

    for( int i=0; i<nr_verts; i++ ) {
        if( vertIdxToPtclIdx[i]>=0 )
            continue;
        const vec3& curPos = poss[i];
        int ptclIdx = (int)tempPtcls.size();
        tempPtcls.push_back( curPos );
        vertIdxToPtclIdx[i] = ptclIdx;
        ptclIdxToVertIdxs.push_back( {i} );
        for( int j=i+1; j<nr_verts; j++ ) {
            if( vertIdxToPtclIdx[j]>=0 )
                continue;
            if( length2(curPos-poss[j]) < 1.0e-6f ) {
                vertIdxToPtclIdx[j] = ptclIdx;
                ptclIdxToVertIdxs[ptclIdx].push_back(j);
            }
        }
    }


    nr_ptcls = (int)tempPtcls.size();
    nr_tris = (int)tris.size();
    ptcl_tris.reserve(nr_tris);
    inv_body_mass = 1.f/bodyMass;
    inv_ptcl_mass = inv_body_mass*nr_ptcls;


    for( int i=0; i<nr_tris; i++ ) {
        uvec3 pTri;
        for( int j=0; j<3; j++ ) {
            pTri[j] = vertIdxToPtclIdx[tris[i][j]];
        }
        ptcl_tris.push_back(pTri);
    }

    idx_verts.resize(nr_ptcls, ivec4(-1));
    idx_verts2.resize(nr_ptcls, ivec4(-1));
    for( int i=0; i<nr_ptcls; i++ ) {
        auto pToV = ptclIdxToVertIdxs[i];
        int nrIdxVs = (int)pToV.size();
        assert(nrIdxVs<=8);
        for( int j=0; j<nrIdxVs; j++ ) {
            if( j<4 ) {
                idx_verts[i][j] = pToV[j];
            }
            else {
                idx_verts2[i][j-4] = pToV[j];
            }
        }
    }
    // end rm same pos verts to ptcls =============



    prev_x_s.reserve(nr_ptcls);
    p_s.reserve(nr_ptcls);
    x_s.reserve(nr_ptcls);
    v_s.reserve(nr_ptcls);
    w_s.reserve(nr_ptcls);

    for( const vec3& p : tempPtcls ) {
        prev_x_s.push_back( p );
        x_s.push_back( p );
        p_s.push_back( p );
        v_s.push_back( vec3{0} );
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
        const uvec3& tri = ptcl_tris[i];
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.y), tri.z, i} );
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.z), tri.y, i} );
        aEdges.push_back( {makeEdgeIdx(tri.y, tri.z), tri.x, i} );
    }
    std::sort(aEdges.begin(), aEdges.end(), [](const Edge& a, const Edge& b) {
        return a.idx_ps.x < b.idx_ps.x || (a.idx_ps.x==b.idx_ps.x && a.idx_ps.y < b.idx_ps.y);
    });

    // cube일땐 -1 안하면 중복 stretch생김
    // cube일때 짧아지는 문제
    for( int i=0; i<nr_tris*3-1; i++ ) {
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
            vec3 e0 = p_s[idxPs.x] - p_s[idxPs.y];
            vec3 e1 = p_s[idxPs.z] - p_s[idxPs.y];
            vec3 e2 = p_s[idxPs.z] - p_s[idxPs.x];
            vec3 e3 = p_s[idxPs.w] - p_s[idxPs.x];
            vec3 e4 = p_s[idxPs.w] - p_s[idxPs.y];
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
            case BT_NONE:
                break;
            case BT_DISTANCE: {
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
            case BT_DIHEDRAL:
                c_dih_bends.push_back( {*this, idxPs} );
                break;
            case BT_ISOMETRIC:
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
    for( const auto& t : ptcl_tris ) {
        volume += glim::signedTetrahedronVolumeTimesSix( p_s[t.x], p_s[t.y], p_s[t.z] );
    }
    return volume/6.f;
}

float SoftBody::getVolumeTimesSix() const {
    float volume = 0;
    for( const auto& t : ptcl_tris ) {
        volume += glim::signedTetrahedronVolumeTimesSix( p_s[t.x], p_s[t.y], p_s[t.z] );
    }
    return volume;
}



// for test individual particle
SoftBody::SoftBody()
    : inv_body_mass(1.f), inv_ptcl_mass(1.f), nr_ptcls(0), nr_tris(0)
{
    constexpr int defalutSize = 50;
    poss.reserve(defalutSize);
    x_s.reserve(defalutSize);
    prev_x_s.reserve(defalutSize);
    p_s.reserve(defalutSize);
    v_s.reserve(defalutSize);
    w_s.reserve(defalutSize);
}
void SoftBody::addPtcl(const vec3& p, float w, const vec3& v) {
    nr_ptcls++;
    poss.push_back( p );
    x_s.push_back( p );
    prev_x_s.push_back( p );
    p_s.push_back( p );
    v_s.push_back( v );
    w_s.push_back( w );
}

void SoftBody::uploadToBuf() {
    int i,j;
    for( i=0; i<nr_ptcls; i++ ) {
        vec3 p = x_s[i];
        uvec4 pToV = idx_verts[i];
        for( j=0; j<4; j++ ) {
            if( pToV[j] == -1 )
                break;
            poss[pToV[j]] = p;
        }
        if( j==4 ) {
            continue;
        }
        pToV = idx_verts2[i];
        for( j=0; j<4; j++ ) {
            if( pToV[j] == -1 )
                break;
            poss[pToV[j]] = p;
        }
    }
    restorePosBuf();
}