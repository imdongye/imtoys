//
//	for generate mesh of general shape
//  2023-01-17 / im dong ye
//
//	uv : upper left
//	st : down left
//
//	todo :
//	1. bumpmap normalmap확인
//	2. https://modoocode.com/129

#ifndef __code_mesh_
#define __code_mesh_

#include "mesh.h"

namespace lim
{
	namespace code_mesh
	{
		Mesh* genQuad(bool genNors = true, bool genUvs = true);
		Mesh* genPlane(int nrSlice = 5, bool genNors = true, bool genUvs = true);
		Mesh* genCube(bool genNors = true, bool genUvs = true);
		// From: http://www.songho.ca/opengl/gl_sphere.html
		// texture coord가 다른 같은 위치의 vertex가 많음
		Mesh* genSphere(const int nrSlices = 50, const int nrStacks = 25, bool genNors = true, bool genUvs = true);
		Mesh* genIcoSphere(const int subdivision = 0, bool genNors = true, bool genUvs = true); // icosahedron, 20면체
		Mesh* genCubeSphere(const int nrSlices = 1, bool genNors = true, bool genUvs = true);
		Mesh* genCubeSphere2(const int nrSlices = 1, bool genNors = true, bool genUvs = true);
		Mesh* genCylinder(const int nrSlices = 50, bool genNors = true, bool genUvs = true);
		Mesh* genCapsule(int nrSlices = 50, int nrStacks = 25, bool genNors = true, bool genUvs = true);
		Mesh* genDonut(int nrSlices = 50, int nrRingVerts = 10, bool genNors = true, bool genUvs = true);
	}
}

#endif
