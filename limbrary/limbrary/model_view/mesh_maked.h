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
	struct MeshQuad : public Mesh { MeshQuad(bool genNors = true, bool genUvs = true); };
	struct MeshPlane : public Mesh { MeshPlane(float radius = 1.f, int nrCols = 5, int nrRows = 5, bool genNors = true, bool genUvs = true); };
	struct MeshCube : public Mesh { MeshCube(bool genNors = true, bool genUvs = true); };
	struct MeshSphere : public Mesh { MeshSphere(float radius = 1.f, int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); };
	struct MeshEnvSphere : public Mesh { MeshEnvSphere(float radius=20.f, int nrSlices = 50, int nrStacks = 25); };
	struct MeshIcoSphere : public Mesh { MeshIcoSphere(int subdivision = 0, bool genNors = true, bool genUvs = true); };
	struct MeshCubeSphere : public Mesh { MeshCubeSphere(int nrSlices = 1, bool genNors = true, bool genUvs = true); };
	struct MeshCubeSphere2 : public Mesh { MeshCubeSphere2(int nrSlices = 1, bool genNors = true, bool genUvs = true); };
	struct MeshCylinder : public Mesh { MeshCylinder(float radius = 1.f, float height = 1.f, int nrSlices = 50, bool genNors = true, bool genUvs = true); };
	struct MeshCapsule : public Mesh { MeshCapsule(int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); };
	struct MeshDonut : public Mesh { MeshDonut(int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true); };
}

#endif
