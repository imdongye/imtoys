#include "pbd.h"

using namespace lim;
using namespace glm;

pbd::SoftBodyGpu* pbd::replaceMeshInModelToSoftBody(ModelView& model, RdNode& nd, int nrShear
    , SoftBody::BendType bendType, float bodyMass, bool refCloseVerts
)
{
    assert(nd.is_local_is_global == false); // do not again

    const Mesh* srcMs = nd.ms;
    Mesh* copiedMeshForMove = new Mesh(*srcMs); // now initGL yet
    mat4 localToWorld = model.getLocalToMeshMtx(srcMs);
    for( vec3& p : copiedMeshForMove->poss ) {
		p = vec3(localToWorld*vec4(p,1));
	}
    SoftBodyGpu* sb = new SoftBodyGpu(std::move(*copiedMeshForMove), nrShear, bendType, bodyMass, refCloseVerts, true);
    sb->initGL();
    nd.ms = sb;
    nd.is_local_is_global = true;
    model.own_meshes_in_view.push_back(sb);
    return sb;

    // todo if skinned mesh then change vertex shader in there material
}