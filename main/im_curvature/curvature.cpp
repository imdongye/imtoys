/*
    인접버텍스순회


*/

#include "curvature.h"
#include <Eigen/Core>
#include <Eigen/QR>
#include <Eigen/LU>
#include <Eigen/SVD>
#include <Eigen/Eigenvalues>

#include <limbrary/using_in_cpp/glm.h>
#include <limbrary/using_in_cpp/std.h>
using namespace lim;

namespace {
    struct AVert
    {
        vec3 p;
        vec3 n;
        float q;
    };
    struct AFace
    {
        ivec3 idxs;
        vec3 n;
    };
    struct HEdge
    {
        const AVert* v0;
        const AVert* v1;
        const AFace* f;
    };
    
    // Mesh 
    vector<AVert> vs;
    vector<AFace> fs;
    vector<HEdge> es;
    vector<vector<const HEdge*>> es_per_vs;
}


void curv::uploadMesh(const Mesh& ms)
{
    vs.reserve(ms.nr_verts);
    fs.reserve(ms.nr_tris);
    es.resize(ms.nr_tris*3);
    es_per_vs.resize(ms.nr_verts);
    for(vector<const HEdge*>& esPerV : es_per_vs) {
        esPerV.reserve(8);
    }


    for(int i=0; i<ms.nr_verts; i++) {
        vs.push_back({ms.poss[i], ms.nors[i], 0.f});
    }
    for(int i=0; i<ms.nr_tris; i++) {
        const ivec3& idxs = ms.tris[i];
        const vec3& p1 = ms.poss[idxs.x];
        const vec3& p2 = ms.poss[idxs.y];
        const vec3& p3 = ms.poss[idxs.z];
        const vec3 n = normalize(cross(p2-p1, p3-p1));
        fs.push_back({idxs, n});

        es.push_back({&vs[idxs.x], &vs[idxs.y], &fs[i]});
        es_per_vs[idxs.x].push_back(&es.back());
        es.push_back({&vs[idxs.y], &vs[idxs.z], &fs[i]});
        es_per_vs[idxs.y].push_back(&es.back());
        es.push_back({&vs[idxs.z], &vs[idxs.x], &fs[i]});
        es_per_vs[idxs.z].push_back(&es.back());
    }
    for(auto& esPerV : es_per_vs) {
        assert(esPerV.empty()==false);
    }
}

void curv::computeCurvature()
{

}

void curv::downloadCurvature(Mesh& ms)
{
    for(int i=0; i<ms.nr_verts; i++) {
        ms.cols[i].r = vs[i].q;
    }
}