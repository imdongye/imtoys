/*
    2024-07-17 / imdongye

*/
#include "pbd.h"

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


pbd::Mesh::Mesh(const lim::Mesh& src) {
    nr_poss = src.poss.size();
    poss.reserve(nr_poss);
    for( const auto& p : src.poss ) {
        poss.push_back( glm::vec4{p,1} );
    }

    nr_tris = src.tris.size();
    tris = src.tris;

    nr_edges = getNrEdges( tris );
    edges.reserve( nr_edges );
    // uvec2 tempEdgeIdx;
    // for( int i=0; i < nr_tris; i++ ) {
    //     for( int j=0; j<nr_tris; j++) {
    //         if( i== j ) {
    //             continue;
    //         }
    //         if( getAdjacentEdge( tris[i], tris[j], tempEdgeIdx ) ) {
    //             edges.emplace_back( tempEdgeIdx, uint() );
    //         }
    //     }
    // } 
    //     for( const auto& t2 : tris ) {
    //         if( &t1 == &t2 ) {
    //             continue;
    //         }
    //         if( getAdjacentEdge( t1, t2, tempEdgeIdx ) ) {
    //             edges.push_back( tempEdgeIdx );
    //         }
    //     }
    // }
}