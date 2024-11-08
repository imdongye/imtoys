/*
    인접버텍스순회


*/

#include "curvature.h"
#include <Eigen/Core>
#include <Eigen/QR>
#include <Eigen/LU>
#include <Eigen/SVD>
#include <Eigen/Eigenvalues>

#include <set>

#include <limbrary/tools/log.h>


#include <limbrary/using_in_cpp/glm.h>
#include <limbrary/using_in_cpp/std.h>
using namespace lim;

namespace {
    struct AVert
    {
        vec3 p;
        vec3 n;
        // 1: max, 2: min
        vec3 cdir1, cdir2;
        float k1, k2;
        float q = 0; // mean curvature
    };
    struct AFace
    {
        ivec3 idxs;
        vec3 n;
    };
    struct HEdge
    {
        const AVert* v1;
        const AVert* v2;
        const AFace* f;
    };
    
    // Mesh 
    int nr_verts;
    int nr_tris;
    vector<AVert> vs;
    vector<AFace> fs;
    vector<HEdge> es;
    vector< vector<const HEdge*> > es_per_vs;

    constexpr int max_adj_verts = 12;
}


void curv::uploadMesh(const Mesh& ms)
{
    nr_verts = ms.nr_verts;
    nr_tris = ms.nr_tris;
    vs.reserve(nr_verts);
    fs.reserve(nr_tris);
    es.reserve(nr_tris*3);
    es_per_vs.resize(nr_verts);
    for( vector<const HEdge*>& esPerV : es_per_vs ) {
        esPerV.reserve(max_adj_verts);
    }


    for( int i=0; i<nr_verts; i++ ) {
        vs.push_back({ms.poss[i], glm::normalize(ms.nors[i])});
    }
    for( int i=0; i<ms.nr_tris; i++ ) {
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
    for( auto& esPerV : es_per_vs ) {
        assert(esPerV.empty()==false);
    }
}

namespace {
    // Copy: vcg curvature_fitting.h
    struct Quadric
    {
        float a, b, c, d, e;

        static Quadric fit(vector<vec3> VV) {
            
        }
    };
}

// Copy: computeCurvature in vcglib/curvature_fitting.h
void curv::computeCurvature()
{
    vector<const AVert*> adj_verts(max_adj_verts);
    vec3 ref[3]; // Todo mat3

    // using in fitQuadric
    vector<vec3> qPs(max_adj_verts);

    for( int i=0; i<nr_verts; i++ ) {
        AVert& v = vs[i];

        adj_verts.clear();
        for( const HEdge* e : es_per_vs[i] ) {
            adj_verts.push_back(e->v2);
        }

        // make vert local space axis in first face by computeReferenceFrames
        {
            const HEdge* e1 = es_per_vs[i][0];
            const AVert& v2 = *e1->v2;
            vec3 projToVn = v2.p - dot(v.n, v2.p-v.p) * v.n; // Todo
            ref[0] = glm::normalize(projToVn - v.p);
            ref[1] = glm::normalize(glm::cross(v.n, ref[0]));
            ref[2] = v.n;
        } 

        // fitQuadric
        float a, b, c, d, e; 
        if( adj_verts.size()>=5 ) {
            qPs.clear();
            for(const AVert* av : adj_verts) {
                vec3 vTang = av->p - v.p;
                qPs.push_back({
                    dot(vTang, ref[0]), 
                    dot(vTang, ref[1]), 
                    dot(vTang, ref[2])
                });
            }
            
            // Quadric::fit
            Eigen::MatrixXf A(qPs.size(),5);
            Eigen::MatrixXf bb(qPs.size(),1);
            Eigen::MatrixXf sol(qPs.size(),1);
            
            for(int c=0; c<qPs.size(); c++) {
                float u = qPs[c].x;
                float v = qPs[c].y;
                float n = qPs[c].z;

                A(c,0) = u*u;
                A(c,1) = u*v;
                A(c,2) = v*v;
                A(c,3) = u;
                A(c,4) = v;

                bb(c,0) = n;
            }
            sol = ((A.transpose()*A).inverse()*A.transpose())*bb;
            a = sol(0,0);
            b = sol(1,0);
            c = sol(2,0);
            d = sol(3,0);
            e = sol(4,0);
        } else {
            a=1;b=1;c=1;d=1;e=1;
        }

        // under fitquadric
        float E = 1.f + d*d;
        float F = d*e;
        float G = 1.f + e*e;   

        vec3 n = glm::normalize(vec3{-d, -e, 1.f});
        v.n = ref[0]*n[0] + ref[1]*n[1] + ref[2]*n[2]; // Todo mat
        
        float L = 2.f * a * n.z;
        float M = b * n.z;
        float N = 2.f * c * n.z;

        Eigen::Matrix2d m;
        m << L*G - M*F, M*E-L*F, M*E-L*F, N*E-M*F;
        m = m / (E*G - F*F);
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> eig(m);

        Eigen::Vector2d c_val = -eig.eigenvalues();
        Eigen::Matrix2d c_vec = eig.eigenvectors();

        vec3 v1(c_vec(0,0), c_vec(0,1), 0);
        vec3 v2(c_vec(1,0), c_vec(1,1), 0);
        v1 = glm::normalize(v1) * float(c_val[0]);
        v2 = glm::normalize(v2) * float(c_val[1]);

        vec3 v1global = ref[0]*v1[0] + ref[1]*v1[1] + ref[2]*v1[2]; // todo mat
        vec3 v2global = ref[0]*v2[0] + ref[1]*v2[1] + ref[2]*v2[2]; // todo mat
        v1global = glm::normalize(v1global);
        v2global = glm::normalize(v2global);

        if( c_val[0] > c_val[1] ) {
            v.cdir1 = v1global;
            v.cdir2 = v2global;
            v.k1 = c_val[0];
            v.k2 = c_val[1];
        } else {
            v.cdir1 = v2global;
            v.cdir2 = v1global;
            v.k1 = c_val[1];
            v.k2 = c_val[0];
        }


        // mean curvature : VertexMeanFromCurvatureDir
        v.q = (v.k1+v.k2) / 2.f;
        // lim::log::pure("%.2f, %.2f, %.2f\n", v.k1, v.k2, v.q);
    }
}

void curv::downloadCurvature(Mesh& ms)
{
    for(int i=0; i<ms.nr_verts; i++) {
        ms.cols[i] = {vs[i].q, vs[i].k1, vs[i].k2};
    }
}