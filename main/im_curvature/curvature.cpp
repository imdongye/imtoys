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
        ivec3 v;
        vec3 n;
    };
    struct FIter
    { 
        AFace* f;
        int vi;

        FIter(const AVert& v) {
            // f = v.
        }
    };
    struct AMesh
    {
        std::vector<AVert> vs;
        std::vector<AFace> fs;
    };
}


void lim::uploadMesh(const Mesh& ms)
{

}

void lim::computeCurvature()
{

}

void lim::downloadCurvature(Mesh& ms)
{

}