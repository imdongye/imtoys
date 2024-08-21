#include "pbd.h"

using namespace lim;
using namespace glm;

pbd::SoftBodyGpu* pbd::replaceMeshInModelToSoftBody(ModelView& model, RdNode::MsSet& msset, int nrShear
    , SoftBody::BendType bendType, float bodyMass, bool refCloseVerts
)
{
    assert(&model!=model.md_data); // if src model then ori ms is orphan mem rick
    assert(msset.transformWhenRender == true); // do not again

    const Mesh* srcMs = msset.ms;
    Mesh* copiedMeshForMove = new Mesh(*srcMs); // now initGL yet
    mat4 localToWorld = model.getLocalToMeshMtx(srcMs);
    for( vec3& p : copiedMeshForMove->poss ) {
		p = vec3(localToWorld*vec4(p,1));
	}
    SoftBodyGpu* sb = new SoftBodyGpu(std::move(*copiedMeshForMove), nrShear, bendType, bodyMass, refCloseVerts, true);
    sb->initGL();
    msset.ms = sb;
    msset.transformWhenRender = false;
    return sb;
}