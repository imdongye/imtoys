/*
	imdongye
	fst 2024-11-07
	end 2024-11-07
*/

#ifndef __curvature_h_
#define __curvature_h_
#include <limbrary/3d/mesh.h>

namespace lim
{
    namespace curv
    {
    void uploadMesh(const Mesh& ms);

    void computeCurvature();
    bool isComputedCurv();

    void downloadCurvature(Mesh& ms); // in color attrib

    Mesh* getClusteredMesh(int vIdx, float threshold, int maxFalseDepth);
    }

}



#endif