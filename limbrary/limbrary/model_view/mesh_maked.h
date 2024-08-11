/*

2023-01-17 / im dong ye
for generate mesh of general shape

uv : upper left
st : down left

Note:
	texture wrap repeat가정
	after construction, you should call inigGL();
	nrStacks is nr Row face lines not verts lines
Todo:
	2. 코드 최적화관련 https://modoocode.com/129
	3. 최소 최대 slice 예외처리
	4. 스피어 덮을 텍스쳐 개수 1개, 2개 선택
	5. 생성자오버로딩으로 radius, inner radius 등으로 생성가능하게
	6
	finished "reserve memory" and "dirrent topology by genUv" list :
	* Quad, Plane, Cloth Cube, Sphere, EnvSphere
	* other is todo

*/

#ifndef __mesh_maked_h_
#define __mesh_maked_h_

#include "mesh.h"

namespace lim
{
	struct MeshQuad : public Mesh { MeshQuad(float width = 1.f, float height = 1.f, bool genNors = true, bool genUvs = true); };
	struct MeshPlane : public Mesh { MeshPlane(float width = 1.f, float height = 1.f, int nrCols = 5, int nrRows = 5, bool genNors = true, bool genUvs = true); };
	struct MeshCloth : public Mesh { MeshCloth(float width = 1.f, float height = 1.f, int nrCols = 5, int nrRows = 5, bool genNors = true, bool genUvs = true); };


	struct MeshCube : public Mesh { MeshCube(float width = 1.f, bool genNors = true, bool genUvs = true); };


	struct MeshSphere : public Mesh { MeshSphere(float width = 1.f, int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); };
	struct MeshEnvSphere : public Mesh { MeshEnvSphere(float width = 20.f, int nrSlices = 50, int nrStacks = 25); }; // inv triangles
	struct MeshIcoSphere : public Mesh { MeshIcoSphere(float width = 1.f, int subdivision = 0, bool genNors = true, bool genUvs = true); };
	
	struct MeshCubeSphere : public Mesh { MeshCubeSphere(float width = 1.f, int nrSlices = 1, bool genNors = true, bool genUvs = true); };
	struct MeshCubeSphereSmooth : public Mesh { MeshCubeSphereSmooth(float width = 1.f, int nrSlices = 1, bool genNors = true, bool genUvs = true); };


	struct MeshCylinder : public Mesh { MeshCylinder(float width = 1.f, float height = 1.f, int nrSlices = 50, bool genNors = true, bool genUvs = true); }; // vertical
	struct MeshCapsule : public Mesh { MeshCapsule(float width = 1.f, float height = 2.f,  int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true); }; // vertical


	struct MeshDonut : public Mesh { MeshDonut(float width = 1.f, float height = 0.4f, int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true); };
	struct MeshDonutWithRadius : public MeshDonut { MeshDonutWithRadius(float outerRadius = 1.f, float innerRadius = 0.4f, int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true); };
	struct MeshDonutWithRadius2 : public MeshDonut { MeshDonutWithRadius2(float donutRadius = 1.f, float thickRadius = 0.4f, int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true); };
}

#endif
