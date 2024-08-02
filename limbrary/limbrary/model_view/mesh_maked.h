/*

2023-01-17 / im dong ye
for generate mesh of general shape

uv : upper left
st : down left
todo :
2. https://modoocode.com/129

*/

#ifndef __mesh_maked_h_
#define __mesh_maked_h_

#include "mesh.h"

namespace lim
{
	struct MeshQuad : public Mesh { MeshQuad(float width = 1.f, bool genNors = true, bool genUvs = true); };
	struct MeshPlane : public Mesh { MeshPlane(float width = 1.f, int nrCols = 5, int nrRows = 5, bool genNors = true, bool genUvs = true); };
	struct MeshCube : public Mesh { MeshCube(float width = 1.f, bool genNors = true, bool genUvs = true); };
	struct MeshSphere : public Mesh { MeshSphere(float width = 1.f, int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); };
	// inv triangles
	struct MeshEnvSphere : public Mesh { MeshEnvSphere(float width = 20.f, int nrSlices = 50, int nrStacks = 25); };
	struct MeshIcoSphere : public Mesh { MeshIcoSphere(float width = 1.f, int subdivision = 0, bool genNors = true, bool genUvs = true); };
	struct MeshCubeSphere : public Mesh { MeshCubeSphere(float width = 1.f, int nrSlices = 1, bool genNors = true, bool genUvs = true); };
	struct MeshCubeSphere2 : public Mesh { MeshCubeSphere2(float width = 1.f, int nrSlices = 1, bool genNors = true, bool genUvs = true); };
	struct MeshCylinder : public Mesh { MeshCylinder(float width = 1.f, float height = 1.f, int nrSlices = 50, bool genNors = true, bool genUvs = true); };
	struct MeshCapsule : public Mesh { MeshCapsule(float width = 1.f, float height = 2.f,  int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); };
	struct MeshDonut : public Mesh { MeshDonut(float width = 1.f, float height = 0.2f, int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true); };
	
	// no seam, shared all verts for set PBD
	struct MeshCubeShared : public Mesh { MeshCubeShared(float width = 1.f, bool withInitGL=true); };
	struct MeshCloth : public Mesh { MeshCloth(glm::vec2 size = {1.f, 1.f}, float innerWidth = 0.1f); };
	struct MeshSphereShared : public Mesh { MeshSphereShared(float width = 1.f, int nrSlices = 50, int nrStacks = 25, bool withInitGL=true); };

}

#endif
