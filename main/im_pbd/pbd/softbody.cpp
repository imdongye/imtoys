#include "pbd.h"
#include <algorithm>
#include <limbrary/g_tools.h>
#include <limbrary/glm_tools.h>
#include <glm/gtx/norm.hpp>

using namespace glm;
using namespace pbd;
using std::vector;


static inline uvec2 makeEdgeIdx(uint a1, uint a2) {
    return (a1<a2)?uvec2{a1,a2}:uvec2{a2,a1};
}

SoftBody::ConstraintParams::ConstraintParams()
    : inv_stiff_dist(0.001f)
    , stretch_pct(1.f), shear_pct(0.8f), bend_pct(0.6f)
    , inv_stiff_dih_bend(46.f), inv_stiff_iso_bend(46.f), inv_stiff_point(0.0006f)
    , inv_stiff_volume(0.01f), pressure(0.0f)
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

RefFrom:
    CreateConstraints() in SoftBodySharedcpp of JoltPhysics
*/
SoftBody::SoftBody(lim::Mesh&& src, int nrShear, BendType bendType
    , float bodyMass, bool refCloseVerts
) : lim::Mesh(std::move(src)), params()
{
    nr_verts = (int)poss.size();
    nr_tris = (int)tris.size();

    // rm same pos verts to ptcls ====================
    if( refCloseVerts ) {
        vector<vec3> tempPtcls;
        tempPtcls.reserve(nr_verts);
        vert_to_ptcl.resize(nr_verts, -1);

        for( int i=0; i<nr_verts; i++ ) {
            if( vert_to_ptcl[i]>=0 )
                continue;
            const vec3& curPos = poss[i];
            const int ptclIdx = (int)tempPtcls.size();
            tempPtcls.push_back( curPos );
            vert_to_ptcl[i] = ptclIdx;

            for( int j=i+1; j<nr_verts; j++ ) {
                if( vert_to_ptcl[j]>=0 )
                    continue;
                if( length2(curPos-poss[j]) < 1.0e-6f ) {
                    vert_to_ptcl[j] = ptclIdx;
                }
            }
        }

        // make ptcl triangle
        ptcl_tris.reserve(nr_tris);
        for( int i=0; i<nr_tris; i++ ) {
            uvec3 pTri;
            for( int j=0; j<3; j++ ) {
                pTri[j] = vert_to_ptcl[tris[i][j]];
            }
            ptcl_tris.push_back(pTri);
        }


        // update member vars
        nr_ptcls = (int)tempPtcls.size();
        prev_x_s = tempPtcls;
        x_s = tempPtcls;
        p_s = tempPtcls;
    } 


    else { // type1 no ref verts ===================
        nr_ptcls = (int)poss.size();

        prev_x_s = poss;
        x_s = poss;
        p_s = poss;
        ptcl_tris = tris;

        // poss and tris will delete in initGL
    }






    inv_body_mass = 1.f/bodyMass;
    inv_ptcl_mass = inv_body_mass*nr_ptcls;
    
    v_s.resize(nr_ptcls, vec3(0));
    debug_dirs.resize(nr_ptcls, vec3(0));
    w_s.resize(nr_ptcls, inv_ptcl_mass);

    struct Edge {
        uvec2 idx_ps;
        uint idx_opp;
        int idx_tri;
    };
    vector<Edge> aEdges; // aside edges

    int nrDupEdges = nr_tris*3;
    aEdges.reserve( nrDupEdges );
    for( int i=0; i<nr_tris; i++ ) {
        const uvec3& tri = ptcl_tris[i];
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.y), tri.z, i} );
        aEdges.push_back( {makeEdgeIdx(tri.x, tri.z), tri.y, i} );
        aEdges.push_back( {makeEdgeIdx(tri.y, tri.z), tri.x, i} );
    }
    std::sort(aEdges.begin(), aEdges.end(), [](const Edge& a, const Edge& b) {
        return a.idx_ps.x < b.idx_ps.x || (a.idx_ps.x==b.idx_ps.x && a.idx_ps.y < b.idx_ps.y);
    });

    // add constraints ======================================
    c_points.reserve(3);

    // Todo: make plane to cloth topology

    for( int i=0; i<nrDupEdges; i++ ) {
        const Edge& edge1 = aEdges[i];
        bool isShear = false;
        for( int j=i+1; j<nrDupEdges; j++ )
        {
            const Edge& edge2 = aEdges[j];

            if( edge1.idx_ps!=edge2.idx_ps ) {
                i = j-1;
                break;
            }
            else if( j==nrDupEdges-1 ) {
                i = nrDupEdges;
            }
            uvec4 idxPs = {edge1.idx_ps, edge1.idx_opp, edge2.idx_opp};
            vec3 e0 = p_s[idxPs.x] - p_s[idxPs.y];
            vec3 e1 = p_s[idxPs.z] - p_s[idxPs.y];
            vec3 e2 = p_s[idxPs.z] - p_s[idxPs.x];
            vec3 e3 = p_s[idxPs.w] - p_s[idxPs.x];
            vec3 e4 = p_s[idxPs.w] - p_s[idxPs.y];
            vec3 n1 = normalize(cross(e1, e0));
            vec3 n2 = normalize(cross(e0, e4));


            e1 = normalize(e1);
            e4 = normalize(e4);
            e2 = normalize(e2);
            e3 = normalize(e3);
            static constexpr float rigthCosThreshold = 0.68f;
            float cosSum = abs(dot(e1, e2)) + abs(dot(e2, e3)) + abs(dot(e3, e4)) + abs(dot(e4, e1));
            // check shear
            if( cosSum < rigthCosThreshold && abs(dot(n1, n2)) > 0.9999f ) {
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

    lim::randomShuffle(c_stretchs);
    lim::randomShuffle(c_shears);
    lim::randomShuffle(c_dist_bends);
    lim::randomShuffle(c_dih_bends);
    lim::randomShuffle(c_iso_bends);

    c_volume.ori_six_volume = getVolumeTimesSix();
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

void SoftBody::initGL(bool withClearMem)
{
    Mesh::initGL(false);
    if( vert_to_ptcl.empty() ) {
        glFinish();
        poss.clear(); poss.shrink_to_fit();
        tris.clear(); tris.shrink_to_fit();
    }
    else if( withClearMem ) {
        // delte mesh data in memory except tris, poss, nors
    }
}

/*
    update normal has 3 types
    1. poss.empty() -> no ref verts, refCloseVerts == false in constructor
    2. update with ptcls
        use p_s to temp storage
    3. update with verts

Todo:
    normal 가중치 면적 크기 비례해야하나?

*/
// type 1 : no ref verts
void SoftBody::updateNorsAndUpload() {
    assert( poss.empty()==true && nors.size()==x_s.size() );

	std::fill(nors.begin(), nors.end(), vec3(0));
	for( const uvec3& t : ptcl_tris )
	{
		vec3 e1 = x_s[t.y] - x_s[t.x];
        vec3 e2 = x_s[t.z] - x_s[t.x];
        vec3 n = normalize(cross(e1, e2));
		nors[t.x] += n;
		nors[t.y] += n;
		nors[t.z] += n;
	}
	for( vec3& n : nors )
	{
		n = normalize(n);
	}


    // upload to gl buf
    assert( buf_pos != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_pos);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, x_s.data());

    assert( buf_nor != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_nor);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, nors.data());
}

// type 2 : update nors with ptcl ( smooth normal )
void SoftBody::updatePossAndNorsWithPtclAndUpload() {
    assert( poss.empty() == false);

    // make normal
    std::fill(p_s.begin(), p_s.end(), vec3(0));
    for( const uvec3& ptri : ptcl_tris ) {
        vec3 e1 = x_s[ptri.y] - x_s[ptri.x];
        vec3 e2 = x_s[ptri.z] - x_s[ptri.x];
        vec3 n = normalize(cross(e1, e2)); // Todo 면적크기
        // temp storage
        p_s[ptri.x] += n;
        p_s[ptri.y] += n;
        p_s[ptri.z] += n;
    }
    for( vec3& n : p_s ) {
        n = normalize(n);
    }

    // update verts
    for( int i=0; i<nr_verts; i++ ) {
        int idxP = vert_to_ptcl[i];
        poss[i] = x_s[idxP];
        nors[i] = p_s[idxP];
    }

    // upload to gl buf
    assert( buf_pos != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_pos);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, poss.data());

    assert( buf_nor != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_nor);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, nors.data());
}

// type 3 : update nors with verts ( face normal )
void SoftBody::updatePossAndNorsWithVertAndUpload() {
    assert( poss.empty() == false);

    // update verts
    for( int i=0; i<nr_verts; i++ ) {
        int idxP = vert_to_ptcl[i];
        poss[i] = x_s[idxP];
    }

    // make nors
    Mesh::updateNorsFromTris();

    // upload to gl buf
    assert( buf_pos != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_pos);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, poss.data());

    assert( buf_nor != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_nor);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, nors.data());
}















// for test individual particle
SoftBody::SoftBody()
    : inv_body_mass(1.f), inv_ptcl_mass(1.f), nr_ptcls(0)
{
    constexpr int defalutSize = 50;
    x_s.reserve(defalutSize);
    prev_x_s.reserve(defalutSize);
    p_s.reserve(defalutSize);
    v_s.reserve(defalutSize);
    w_s.reserve(defalutSize);
    debug_dirs.reserve(defalutSize);
}
void SoftBody::addPtcl(const vec3& p, float w, const vec3& v) {
    nr_ptcls++;
    x_s.push_back( p );
    prev_x_s.push_back( p );
    p_s.push_back( p );
    v_s.push_back( v );
    w_s.push_back( w );
    debug_dirs.push_back( vec3(0) );
}

