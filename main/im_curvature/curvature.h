/*
	imdongye
	fst 2024-11-07
	end 2024-11-07
*/

#ifndef __curvature_h_
#define __curvature_h_
#include <limbrary/model_view/mesh.h>

namespace lim
{
    namespace curv
    {
    void uploadMesh(const Mesh& ms);

    void computeCurvature();

    void downloadCurvature(Mesh& ms); // in color attrib
    }

}



#endif