#include <limbrary/3d/mesh_maked.h>
#include <limbrary/texture.h>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <limbrary/tools/glim.h>
#include <limbrary/tools/text.h>

using namespace lim;
using namespace glm;




namespace {
	inline ivec2 makeEdgeIdx(int a1, int a2) {
		return (a1<a2)?ivec2{a1,a2}:ivec2{a2,a1};
	}
	struct SubEdge {
		ivec2 idx_vs;
		int idx_inter;
	};
	int getIdxInter( const std::vector<SubEdge>& subEdges, const ivec2& edgeIdx ) {
		for( const SubEdge& subEdge : subEdges ) {
			if( subEdge.idx_vs == edgeIdx ) {
				return subEdge.idx_inter;
			}
		}
		return -1;
	}
	Mesh::VertBoneInfo blendBoneInfos( const Mesh::VertBoneInfo& a, const Mesh::VertBoneInfo& b ) {
		int nrUsedBones = 0;
		std::pair<int, float> idxWeights[8];

		// 1. weighted sum by idx
		for(int i=0; i<4; i++) {
			int idx = a.idxs[i];
			float weight = a.weights[i];
			int targetIdx = 0;
			for(; targetIdx<nrUsedBones; targetIdx++) {
				if( idxWeights[targetIdx].first == idx )
					break;
			}
			if( targetIdx==nrUsedBones ) {
				nrUsedBones++;
				idxWeights[targetIdx] = {idx, weight};
			}
			idxWeights[targetIdx].second += weight;
		}
		for(int i=0; i<4; i++) {
			int idx = b.idxs[i];
			float weight = b.weights[i];
			int targetIdx = 0;
			for(; targetIdx<nrUsedBones; targetIdx++) {
				if( idxWeights[targetIdx].first == idx )
					break;
			}
			if( targetIdx==nrUsedBones ) {
				nrUsedBones++;
				idxWeights[targetIdx] = {idx, weight};
			}
			idxWeights[targetIdx].second += weight;
		}

		// 2. sort by weight
		if( nrUsedBones>4 ) {
			for( int i=0; i<nrUsedBones; i++ ) {
				for( int j=i+1; j<nrUsedBones; j++ ) {
					if( idxWeights[i].second < idxWeights[j].second ) {
						std::swap(idxWeights[i], idxWeights[j]);
					}
				}
			}
		}

		// 3. get only 4
		float totalWeight = 0.f;
		for( int i=0; i<4; i++ ) {
			totalWeight += idxWeights[i].second;
		}
		Mesh::VertBoneInfo rst;
		for( int i=0; i<4; i++ ) {
			rst.idxs[i] = idxWeights[i].first;
			rst.weights[i] = idxWeights[i].second / totalWeight;
		}
		return rst;
	}
}

/*
   tz
pz/  \py
 /    \ 
tx----ty
   px  
*/
void Mesh::subdivide(int level)
{
	if( level == 0 )
		return;

	if( tris.size()*4 > 10000 ) {
		log::err("too many subdivide triangles");
		return;
	}
	
	std::vector<SubEdge> subEdges;
	subEdges.reserve(tris.size()*3);
	std::vector<ivec3> srcTris = tris;
	tris.clear();
	tris.reserve(srcTris.size()*4);

	int idxNewVert = poss.size();
	for( ivec3 t : srcTris )
	{
		ivec3 p;
		ivec2 edges[3] = {
			makeEdgeIdx(t.x, t.y),
			makeEdgeIdx(t.y, t.z),
			makeEdgeIdx(t.z, t.x),
		};

		// make inter vert idxs
		for( int i=0; i<3; i++ )
		{
			ivec2 e = edges[i];
			int idxInter = getIdxInter( subEdges, e );
			if( idxInter<0 ) {
				idxInter = idxNewVert++;
				subEdges.push_back({edges[i], idxInter});
			}
			p[i] = idxInter;
		}

		// add subdivided triangles
		tris.push_back({t.x, p.x, p.z});
		tris.push_back({p.x, t.y, p.y});
		tris.push_back({p.z, p.y, t.z});
		tris.push_back({p.x, p.y, p.z});
	}

	// make inter verts
	nr_verts = idxNewVert;
	poss.resize(nr_verts);
	if( nors.size()>0 ) 	  nors.resize(idxNewVert);
	if( uvs.size()>0 ) 		  uvs.resize(idxNewVert);
	if( cols.size()>0 ) 	  cols.resize(idxNewVert);
	if( tangents.size()>0 )   tangents.resize(idxNewVert);
	if( bitangents.size()>0 ) bitangents.resize(idxNewVert);
	if( bone_infos.size()>0 ) bone_infos.resize(idxNewVert);

	for( SubEdge e : subEdges ) {
		poss[e.idx_inter] = 0.5f*(poss[e.idx_vs.x]+poss[e.idx_vs.y]);
		if( nors.size()>0 ) 	  nors[e.idx_inter] = normalize(0.5f*(nors[e.idx_vs.x]+nors[e.idx_vs.y]) );
		if( uvs.size()>0 ) 		  uvs[e.idx_inter] = 0.5f*(uvs[e.idx_vs.x]+uvs[e.idx_vs.y]);
		if( cols.size()>0 ) 	  cols[e.idx_inter] = 0.5f*(cols[e.idx_vs.x]+cols[e.idx_vs.y]);
		if( tangents.size()>0 )   tangents[e.idx_inter] = 0.5f*(tangents[e.idx_vs.x]+tangents[e.idx_vs.y]);
		if( bitangents.size()>0 ) bitangents[e.idx_inter] = 0.5f*(bitangents[e.idx_vs.x]+bitangents[e.idx_vs.y]);
		if( bone_infos.size()>0 ) bone_infos[e.idx_inter] = blendBoneInfos(bone_infos[e.idx_vs.x], bone_infos[e.idx_vs.y]);
		// Todo: tangent and bitangent is not ensure orthogonal.
	}

	subdivide(level-1);
}


void Mesh::updateNorsFromTris()
{
	assert( nors.size()==poss.size() );

	std::fill(nors.begin(), nors.end(), vec3(0));
	for( const ivec3& t : tris )
	{
		vec3 e1 = poss[t.y] - poss[t.x];
        vec3 e2 = poss[t.z] - poss[t.x];
        vec3 n = normalize(cross(e1, e2));
		nors[t.x] += n;
		nors[t.y] += n;
		nors[t.z] += n;
	}
	for( vec3& n : nors )
	{
		n = normalize(n);
	}
}





MeshQuad::MeshQuad(float width, float height, bool genNors, bool genUvs)
{
	name = "quad";

	const float hHalf = width*0.5f;
	const float vHalf = height*0.5f;

	nr_verts = 4;
	poss.reserve(nr_verts);
	poss.push_back({-hHalf, vHalf, 0});
	poss.push_back({hHalf, vHalf, 0});
	poss.push_back({-hHalf, -vHalf, 0});
	poss.push_back({hHalf, -vHalf, 0});

	if( genNors ) {
		nors.reserve(nr_verts);
		nors.push_back(glim::front);
		nors.push_back(glim::front);
		nors.push_back(glim::front);
		nors.push_back(glim::front);
	}
	
	if( genUvs ) {
		uvs.reserve(nr_verts);
		uvs.push_back({0,1});
		uvs.push_back({1,1});
		uvs.push_back({0,0});
		uvs.push_back({1,0});
	}

	nr_tris = 2;
	tris.reserve(nr_tris);
	tris.push_back({0,3,1});
	tris.push_back({0,2,3});
}





MeshPlane::MeshPlane(float width, float height, int nrCols, int nrRows, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("plane_%d_%d", nrCols, nrRows);
	const float fNrCols = nrCols;
	const float fNrRows = nrRows;
	const float startX = -width / 2.f;
	const float startY = -height / 2.f;
	const float stepX = width / fNrCols;
	const float stepY = height / fNrRows;


	nr_verts = (nrRows+1)*(nrCols+1);
	poss.reserve(nr_verts);
	if( genNors ) {
		nors.reserve(nr_verts);
	}
	if( genUvs ) {
		uvs.reserve(nr_verts);
	}

	for(int i = 0; i <= nrRows; i++) for(int j = 0; j <= nrCols; j++)
	{
		poss.push_back({startX + stepX * j, 0, startY + stepY * i});
		if( genNors ) {
			nors.push_back( glim::up );
		}
		if( genUvs ) {
			uvs.push_back({ j/fNrCols, (fNrRows-i)/fNrRows });
		}
	}

	const int nrColVerts = nrCols+1;
	nr_tris = nrRows*nrCols*2;
	tris.reserve(nr_tris);
	for(int i = 0; i < nrRows; i++) for(int j = 0; j < nrCols; j++)
	{
		// 0-1
		// |\|
		// 2-3
		const int ori = i * nrColVerts + j;

		// lower
		tris.push_back({ori + 0,
						ori + 0 + nrColVerts,
						ori + 1 + nrColVerts});
		// upper
		tris.push_back({ori + 0,
						ori + 1 + nrColVerts,
						ori + 1});
	}
}





MeshCloth::MeshCloth(float width, float height, int nrCols, int nrRows, bool genNors, bool genUvs) {
	name = fmtStrToBuf("plane_%d_%d", nrCols, nrRows);
	const float fNrCols = nrCols;
	const float fNrRows = nrRows;
	const float startX = -width / 2.f;
	const float startY = -height / 2.f;
	const float stepX = width / fNrCols;
	const float stepY = height / fNrRows;


	nr_verts = (nrRows+1)*(nrCols+1);
	poss.reserve(nr_verts);
	if( genNors ) {
		nors.reserve(nr_verts);
	}
	if( genUvs ) {
		uvs.reserve(nr_verts);
	}

	for(int i = 0; i <= nrRows; i++) for(int j = 0; j <= nrCols; j++)
	{
		poss.push_back({startX + stepX * j, 0, startY + stepY * i});
		if( genNors ) {
			nors.push_back( glim::up );
		}
		if( genUvs ) {
			uvs.push_back({ j/fNrCols, (fNrRows-i)/fNrRows });
		}
	}

	const int nrColVerts = nrCols+1;
	nr_tris = nrRows*nrCols*2;
	tris.reserve(nr_tris);
	for( int i=0; i<nrRows; i++ ) for( int j=0; j<nrCols; j++ )
	{
		const int ori = j + nrColVerts*i;
		// 0-1
		// |\|
		// 2-3
		if( (i+j)%2==0 ) {
			// upper
			tris.push_back({ori + 0,
							ori + 1 + nrColVerts,
							ori + 1});
			// lower
			tris.push_back({ori + 0,
							ori + 0 + nrColVerts,
							ori + 1 + nrColVerts});
		}
		// 0-1
		// |/|
		// 2-3
		else {
			// upper
			tris.push_back({ori + 0,
							ori + 0 + nrColVerts,
							ori + 1});
			// lower
			tris.push_back({ori + 1,
							ori + 0 + nrColVerts,
							ori + 1 + nrColVerts});
		}
	}
}






// input : counterclockwise
static void addTriFaceFromQuad(ivec4 quad, std::vector<ivec3>& tris) {
	tris.emplace_back(quad.x, quad.y, quad.z);
	tris.emplace_back(quad.x, quad.z, quad.w);
}

MeshCube::MeshCube(float width, bool genNors, bool genUvs)
{
	const float half = width*0.5f;

	if( genNors || genUvs )
	{
		name = "cube";
		const vec3 cbNors[6] = {
			{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
		const vec3 cbTans[6] = {
			{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};


		nr_verts = 24;
		poss.reserve(nr_verts);
		if( genNors ) {
			nors.reserve(nr_verts);
		}
		if( genUvs ) {
			uvs.reserve(nr_verts);
		}

		for(int i = 0; i < 6; i++)
		{
			vec3 n = cbNors[i];
			vec3 t = cbTans[i];
			vec3 b = cross(n, t);

			poss.push_back(  t*half + b*half + n*half );
			poss.push_back( -t*half + b*half + n*half );
			poss.push_back( -t*half - b*half + n*half );
			poss.push_back(  t*half - b*half + n*half );

			if( genNors ) {
				nors.push_back(n);
				nors.push_back(n);
				nors.push_back(n);
				nors.push_back(n);
			}

			if( genUvs ) {
				uvs.push_back({1,1});
				uvs.push_back({0,1});
				uvs.push_back({0,0});
				uvs.push_back({1,0});
			}
		}

		nr_tris = 6*2;
		tris.reserve(nr_tris);
		for(unsigned int i = 0; i < 6; i++)
		{
			tris.push_back({ 0+i*4, 1+i*4, 2+i*4 });
			tris.push_back({ 0+i*4, 2+i*4, 3+i*4 });
		}
	}
	else
	{
		name = "shared verts cube";
		/*
			4 5
			6 7

			0 1
			2 3
		*/
		poss.reserve(8);
		for( int y=0; y<2; y++) for( int z=0; z<2; z++) for( int x=0; x<2; x++) {
			poss.emplace_back( (x-0.5f)*width, (y-0.5f)*width, (z-0.5f)*width );
		}
		tris.reserve(6*2);
		addTriFaceFromQuad({0,1,3,2}, tris);
		addTriFaceFromQuad({6,7,5,4}, tris);
		addTriFaceFromQuad({2,3,7,6}, tris);
		addTriFaceFromQuad({3,1,5,7}, tris);
		addTriFaceFromQuad({1,0,4,5}, tris);
		addTriFaceFromQuad({0,2,6,4}, tris);
	}
}







// From: http://www.songho.ca/opengl/gl_sphere.html
// texture coord가 다른, 같은 위치의 버텍스가 많음 특히 위아래 버텍스
// phi : angle form xy-plane [pi/2, -pi/2] <= top down
// theta : y-axis angle [0, 2pi]
MeshSphere::MeshSphere(float width, int nrSlices, int nrStacks, bool genNors, bool genUvs)
{
	const float radius = width*0.5f;
	const float fNrSlices = (float)nrSlices;
	const float fNrStacks = (float)nrStacks;

	name = fmtStrToBuf("sphere_sl%d_st%d", nrSlices, nrStacks);

	nr_tris = nrSlices*2 + (nrStacks-2)*nrSlices*2;
	tris.reserve(nr_tris);

	if( genUvs ) // has seams
	{
		nr_verts = 2*nrSlices + 2*(nrSlices+1)*(nrStacks-1);
		poss.reserve(nr_verts);
		uvs.reserve(nr_verts);
		if( genNors )
			nors.reserve(nr_verts);

		// 1. make inter triangles
		for( int stack = 1; stack < nrStacks; stack++ )
		{
			float phi = glim::pi90 - glim::pi * stack / fNrStacks;
			float y = sin(phi);
			float cosPhi = cos(phi);
			for( int slice = 0; slice <= nrSlices; slice++ )
			{
				float theta = glim::pi2 * slice / fNrSlices;
				float x = cosPhi * cos(theta);
				float z = -cosPhi * sin(theta);
				vec3 nor = {x, y, z};
				poss.push_back(nor*radius);
				uvs.push_back({ 2.f*slice/fNrSlices, 1.f - stack/fNrStacks });
				if( genNors ) {
					nors.push_back(nor);
				}
			}
		}
		for( int stack = 0; stack < nrStacks-2; stack++ )
		{
			int curRow = (nrSlices+1)*stack;
			int nextRow = (nrSlices+1)*(stack+1);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
			/*
				cc nc
				cn nn
			*/
				int curCol = slice;
				int nextCol = slice+1;
				tris.push_back({ curCol+curRow, nextCol+nextRow, nextCol+curRow });
				tris.push_back({ curCol+curRow, curCol+nextRow, nextCol+nextRow });
			}
		}


		// 2. make top and bot triangles
		int topIdx = poss.size();
		for( int slice = 0; slice < nrSlices; slice++ ) {
			poss.push_back({0, radius, 0});
			uvs.push_back({ 2.f*slice/fNrSlices, 1.f });
			if( genNors )
				nors.push_back({0,1,0});
		}
		int botIdx = poss.size();
		for( int slice = 0; slice < nrSlices; slice++ ) {
			poss.push_back({0, -radius, 0});
			uvs.push_back({ 2.f*slice/fNrSlices, 0.f });
			if( genNors )
				nors.push_back({0,-1,0});
		}

		int topRow = 0;
		int botRow = (nrSlices+1)*(nrStacks-2);
		for( int slice = 0; slice < nrSlices; slice++ ) {
			int curCol = slice;
			int nextCol = slice+1;
			tris.push_back({ curCol+topRow, nextCol+topRow, topIdx+slice });
			tris.push_back({ nextCol+botRow, curCol+botRow, botIdx+slice });
		}
	}
	else // no seams, all shared verts mesh
	{
		nr_verts = (nrSlices)*(nrStacks-1)+2;
		poss.reserve(nr_verts);
		if( genNors ) {
			nors.reserve(nr_verts);
		}

		// 1. make inter triangles
		for( int stack = 1; stack < nrStacks; stack++ )
		{
			float phi = glim::pi90 - glim::pi * stack / fNrStacks;
			float y = sin(phi);
			float cosPhi = cos(phi);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
				float theta = glim::pi2 * slice / fNrSlices;
				float x = cosPhi * cos(theta);
				float z = -cosPhi * sin(theta);
				vec3 nor = {x, y, z};
				poss.push_back(nor*radius);
				if( genNors ) {
					nors.push_back(nor);
				}
			}
		}
		for( int stack = 0; stack < nrStacks-2; stack++ )
		{
			int curRow = nrSlices*stack;
			int nextRow = nrSlices*(stack+1);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
			/*
				cc nc
				cn nn
			*/
				int curCol = slice;
				int nextCol = (slice==nrSlices-1) ? 0 : slice+1;
				tris.push_back({ curCol+curRow, nextCol+nextRow, nextCol+curRow });
				tris.push_back({ curCol+curRow, curCol+nextRow, nextCol+nextRow });
			}
		}
		

		// 2. make top and bot triangles
		int topIdx = poss.size();
		int botIdx = topIdx + 1;
		poss.push_back({0, radius, 0});
		poss.push_back({0, -radius, 0});
		if( genNors ) {
			nors.push_back({0, 1,0});
			nors.push_back({0,-1,0});
		}

		int topRow = 0;
		int botRow = nrSlices*(nrStacks-2);
		for( int slice = 0; slice < nrSlices; slice++ ) {
			int curCol = slice;
			int nextCol = (slice==nrSlices-1) ? 0 : slice+1;
			tris.push_back({ curCol+topRow, nextCol+topRow, topIdx });
			tris.push_back({ nextCol+botRow, curCol+botRow, botIdx });
		}
	}
}





// Sphere reserce triangle version
MeshEnvSphere::MeshEnvSphere(float width, int nrSlices, int nrStacks)
{
	name = fmtStrToBuf("env_sphere_sl%d_st%d", nrSlices, nrStacks);

	const float radius = width*0.5f;
	const float fNrSlices = (float)nrSlices;
	const float fNrStacks = (float)nrStacks;

	nr_verts = 2*nrSlices + 2*(nrSlices+1)*(nrStacks-1);
	nr_tris = nrSlices*2 + (nrStacks-2)*nrSlices*2;
	poss.reserve(nr_verts);
	uvs.reserve(nr_verts);
	tris.reserve(nr_tris);

	// 1. make inter triangles
	for( int stack = 1; stack < nrStacks; stack++ )
	{
		float phi = glim::pi90 - glim::pi * stack / fNrStacks;
		float y = sin(phi);
		float cosPhi = cos(phi);
		for( int slice = 0; slice <= nrSlices; slice++ )
		{
			float theta = glim::pi2 * slice / fNrSlices;
			float x = cosPhi * cos(theta);
			float z = -cosPhi * sin(theta);
			vec3 nor = {x, y, z};
			poss.push_back(nor*radius);
			uvs.push_back({ 1.f - slice/fNrSlices, 1.f-stack/fNrStacks }); // u reverse also
		}
	}
	for( int stack = 0; stack < nrStacks-2; stack++ )
	{
		int curRow = (nrSlices+1)*stack;
		int nextRow = (nrSlices+1)*(stack+1);
		for( int slice = 0; slice < nrSlices; slice++ )
		{
		/*
			cc nc
			cn nn
		*/
			int curCol = slice;
			int nextCol = slice+1;
			tris.push_back({ curCol+curRow, nextCol+curRow, nextCol+nextRow });
			tris.push_back({ curCol+curRow, nextCol+nextRow, curCol+nextRow });
		}
	}


	// 2. make top and bot triangles
	int topIdx = poss.size();
	for( int slice = 0; slice < nrSlices; slice++ ) {
		poss.push_back({0, radius, 0});
		uvs.push_back({ 1.f - slice/fNrSlices, 1.f });
	}
	int botIdx = poss.size();
	for( int slice = 0; slice < nrSlices; slice++ ) {
		poss.push_back({0, -radius, 0});
		uvs.push_back({ 1.f - slice/fNrSlices, 0.f });
	}

	int topRow = 0;
	int botRow = (nrSlices+1)*(nrStacks-2);
	for( int slice = 0; slice < nrSlices; slice++ ) {
		int curCol = slice;
		int nextCol = slice+1;
		tris.push_back({ nextCol+topRow, curCol+topRow, topIdx+slice });
		tris.push_back({ curCol+botRow, nextCol+botRow, botIdx+slice });
	}
}






// icosahedron, 20면체 => icosphere/geosphere
// From: https://www.songho.ca/opengl/gl_sphere.html
// todo: now reserve is not consider subdivision
MeshIcoSphere::MeshIcoSphere(float width, int subdivision, bool genNors, bool genUvs)
{
	if( subdivision > 4 ) {
		log::warn("subdivision is too big in ico sphere");
		subdivision = 4;
	}

	name = fmtStrToBuf("icosphere_s%d", subdivision);
	const float radius = width*0.5f;
	const float halfH = radius * 0.5f;	 // half height
	const float aStep = glim::pi2 / 5.f; // angle step
	const float base = halfH * sqrtf(3); // sliced triangle height
	float topA = 0; // first row angle
	float botA = aStep * 0.5f; // second row angle
	
	nr_tris = 4*5;
	tris.reserve(nr_tris);

	if( genUvs )
	{
		nr_verts = 2*5 + 2*6;
		poss.reserve(nr_verts);
		uvs.reserve(nr_verts);
		if( genNors ) {
			nors.reserve(nr_verts);
		}

		const float uStep = 1.f / 11.f;
		const float vStep = 1.f / 3.f;

		// 1. top and bot vertexs
		for (int i = 0; i < 5; i++)
		{
			poss.push_back({0, radius,0});
			poss.push_back({0,-radius,0});
			uvs.push_back({ uStep*(1+ 2*i), 1 });
			uvs.push_back({ uStep*(3+ 2*i), 0 });
			if( genNors ) {
				nors.push_back({0, 1,0});
				nors.push_back({0,-1,0});
			}
		}

		// 2. two rows
		for (int i = 0; i < 6; i++)
		{
			poss.push_back({ base*cos(topA), halfH, -base*sin(topA) });
			uvs.push_back({ uStep*(2*i), 2*vStep });
			if(genNors) nors.push_back( normalize(poss.back()) );
			topA += aStep;

			poss.push_back({ base*cos(botA), -halfH, -base*sin(botA)});
			uvs.push_back({ uStep*(1+2*i), vStep });
			if(genNors)	nors.push_back( normalize(poss.back()) );
			botA += aStep;
		}

		// hard coded triangle indexes
		// top row
		tris.push_back({0,  10, 12});
		tris.push_back({2,  12, 14});
		tris.push_back({4,  14, 16});
		tris.push_back({6,  16, 18});
		tris.push_back({8,  18, 20});

		// inter top row
		tris.push_back({10, 11, 12});
		tris.push_back({12, 13, 14});
		tris.push_back({14, 15, 16});
		tris.push_back({16, 17, 18});
		tris.push_back({18, 19, 20});

		// inter bot row
		tris.push_back({13, 12, 11});
		tris.push_back({15, 14, 13});
		tris.push_back({17, 16, 15});
		tris.push_back({19, 18, 17});
		tris.push_back({21, 20, 19});

		// bot row
		tris.push_back({1,  13, 11});
		tris.push_back({3,  15, 13});
		tris.push_back({5,  17, 15});
		tris.push_back({7,  19, 17});
		tris.push_back({9,  21, 19});
	}
	else
	{
		poss.push_back({0, radius,0});
		poss.push_back({0,-radius,0});
		if( genNors ) {
			nors.push_back({0, 1,0});
			nors.push_back({0,-1,0});
		}
		const int topIdx = 0;
		const int botIdx = 1;
		int fstRowIdx = 2; // += 2
		int	scdRowIdx = 3; // += 2
		for (int i = 0; i < 5; i++)
		{
			poss.push_back({ base*cos(topA), halfH, -base*sin(topA) });
			if(genNors) nors.push_back( normalize(poss.back()) );
			topA += aStep;

			poss.push_back({ base*cos(botA), -halfH, -base*sin(botA)});
			if(genNors)	nors.push_back( normalize(poss.back()) );
			botA += aStep;

			int nextFstRowIdx = (i==4) ? 2 : fstRowIdx + 2;
			int nextScdRowIdx = (i==4) ? 3 : scdRowIdx + 2;
			tris.push_back({topIdx, fstRowIdx, nextFstRowIdx});
			tris.push_back({fstRowIdx, scdRowIdx, nextFstRowIdx});
			tris.push_back({nextFstRowIdx, scdRowIdx, nextScdRowIdx});
			tris.push_back({botIdx, nextScdRowIdx, scdRowIdx});
			fstRowIdx+=2;
			scdRowIdx+=2;
		}
	}
	
	for(int i = 0; i < subdivision; i++) {
		int newPosIdx = poss.size();
		subdivide(1);
		int newPosEndIdx = poss.size();
		for(;newPosIdx<newPosEndIdx; newPosIdx++) {
			poss[newPosIdx] = radius * normalize(poss[newPosIdx]);
		}
	}
}






MeshCubeSphere::MeshCubeSphere(float width, int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("cubesphere_s%d", nrSlices);

	const float radius = width*0.5f;

	const vec3 cbNors[6] = {
		{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
	const vec3 cbTans[6] = {
		{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

	for (int side = 0; side < 6; side++)
	{
		vec3 n = cbNors[side];
		vec3 t = cbTans[side];
		vec3 b = cross(n, t);

		int offset = poss.size();

		for( int y=0; y<=nrSlices; y++ ) for( int x=0; x<=nrSlices; x++ ) 
		{
			float dx = x/(float)nrSlices*2.f - 1.f;
			float dy = y/(float)nrSlices*2.f - 1.f;
			vec3 nor = normalize( n + t*dx + b*dy );
			poss.push_back( nor*radius );
			if( genNors ) nors.push_back(nor);
			if( genUvs )  uvs.push_back({ dx*0.5f+0.5f, dy*0.5f+0.5f });
		}

		const int nrCols = nrSlices + 1;
		for (int y = 0; y < nrSlices; y++)
		{
			const int curRow = offset + y * nrCols;
			const int nextRow = offset + (y + 1) * nrCols;
			for (int x = 0; x < nrSlices; x++)
			{
				tris.push_back({ nextRow+x, curRow+x, 	curRow+x+1  });
				tris.push_back({ nextRow+x, curRow+x+1, nextRow+x+1 });
			}
		}
	}
}






// smooth version
MeshCubeSphereSmooth::MeshCubeSphereSmooth(float width, int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("smoothcubesphere_s%d", nrSlices);
	
	
	const float radius = width*0.5f;
	const vec3 cbNors[6] = {
		{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
	const vec3 cbTans[6] = {
		{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

	/* genUnitCubeSpherePositiveXFace */
	std::vector<vec3> facePoints;

	for( int y=0; y<=nrSlices; y++ ) { // z-axis angle
		float phi = glim::pi90 * y/(float)nrSlices - glim::pi45;
		vec3 n1 = { -sin(phi), cos(phi), 0 };
		for( int x=0; x<=nrSlices; x++ ) { // y-axis angle
			float theta = glim::pi90 * (float)x / nrSlices - glim::pi45;
			vec3 n2 = { sin(theta), 0, cos(theta) };
			facePoints.push_back(normalize(cross(n1, n2)));
		}
	}

	for( int side=0; side<6; side++ ) {
		const int nrCols = nrSlices + 1;
		const int offset = poss.size();
		const mat3 rotMat = mat3(cbNors[side], cbTans[side], cross(cbNors[side], cbTans[side]));

		for( int y=0; y<=nrSlices; y++ ) for(int x=0; x<=nrSlices; x++ ) {
			vec3 nor = rotMat * facePoints[nrCols * y + x];
			poss.push_back(nor*radius);
			if( genNors ) nors.push_back(nor);
			if( genUvs )  uvs.push_back({ x/(float)nrSlices, y/(float)nrSlices });
		}

		for( int y=0; y<nrSlices; y++ ) {
			const int curRow = offset + y*nrCols;
			const int upRow = offset + (y+1)*nrCols;
			for( int x=0; x<nrSlices; x++ ) {
				tris.push_back({upRow+x, curRow+x, curRow+x+1});
				tris.push_back({upRow+x, curRow+x+1, upRow+x+1});
			}
		}
	}
}





MeshCone::MeshCone(float width, float height, int nrSlices, bool genNors, bool genUvs)
{
	const float radius = width*0.5f;
	name = fmtStrToBuf("cone_s%d", nrSlices);
	
	const float half = height*0.5f;

	// idx0 : bot center
	poss.push_back({ 0,-half,0 }); // bot 0
	if( genNors )
		nors.push_back({ 0,-1,0 });
	if( genUvs )
		uvs.push_back({ .5f, .5f });

	// idx1 or 1~ : top center
	if( !genNors && !genUvs ) {
		poss.push_back({ 0,half,0 }); // top 1
		for(int i=0; i<=nrSlices; i++) {
			float pct = i/float(nrSlices);
			float phi = glim::pi2 * pct;
			poss.push_back({ radius*cos(phi), -half, radius*sin(phi) }); // bot outside ring
		}

		for(int i=0; i<nrSlices; i++) {
			int ringIdx = i+2;
			tris.push_back({ 1, ringIdx+1, ringIdx   }); // side
			tris.push_back({ 0, ringIdx  , ringIdx+1 }); // bot
		}

		return;
	}

	// else have uv or nor

	float theta = glm::atan(height/radius);

	for(int i=0; i<=nrSlices; i++) {
		float pct = i/float(nrSlices);
		float phi = glim::pi2 * pct;
		vec3 temp = { radius*cos(phi), -half, radius*sin(phi) };
		poss.push_back({ 0,half,0 }); // top 1 + 3*i
		poss.push_back( temp ); // side ring for side tris
		poss.push_back( temp ); // side ring for bot
		if( genNors ) {
			// 직삼각형으로 만든 정사각형의 내부도 정사각형
			// temp.y += 2.f*half;
			// vec3 nor = glm::normalize(temp);
			// 구면좌표계
			vec3 nor = { cos(theta) * cos(phi), sin(theta), cos(theta) * sin(phi) };
			nors.push_back( nor );
			nors.push_back( nor );
			nors.push_back( {0, -1, 0} );
		}
		if( genUvs ) {
			vec2 ring = 0.5f*vec2(cos(phi), sin(phi)) + vec2(0.5f, 0.5f);
			uvs.push_back({ pct, 1.f });
			uvs.push_back({ pct, 0.f });
			uvs.push_back( ring );
		}
	}
	for(int i=0; i<nrSlices; i++) {
		int curIdx = i*3;
		int nextIdx = (i+1)*3;

		tris.push_back({ 1+curIdx, 2+nextIdx, 2+curIdx }); // side
		tris.push_back({ 0, 3+curIdx, 3+nextIdx }); // bot
	}
}






MeshCylinder::MeshCylinder(float width, float height, int nrSlices, bool genNors, bool genUvs)
{
	const float radius = width*0.5f;
	name = fmtStrToBuf("sylinder_s%d", nrSlices);
	
	const float half = height*0.5f;

	poss.push_back({ 0, half,0 });
	poss.push_back({ 0,-half,0 });
	if( genNors ) {
		nors.push_back({ 0, 1,0 });
		nors.push_back({ 0,-1,0 });
	}
	if( genUvs ) {
		uvs.push_back({ .5f, .5f });
		uvs.push_back({ .5f, .5f });
	}

	for( int i=0; i<=nrSlices; i++ )
	{
		float theta = glim::pi2*i/(float)nrSlices;
		float x =  radius*cos(theta);
		float z = -radius*sin(theta);
		vec3 pos;
		vec3 sideNor = normalize(vec3(x, 0, z));
		float sideU = fract( 2.f * i / (float)nrSlices );
		vec2 circleUv = {x, -z};
		circleUv = .5f * (circleUv + vec2(1));

		pos = {x, half, z};
		poss.push_back(pos);
		poss.push_back(pos);

		pos = {x, -half, z};
		poss.push_back(pos);
		poss.push_back(pos);

		if( genNors ) {
			nors.push_back({ 0, 1,0 });
			nors.push_back(sideNor);
			nors.push_back(sideNor);
			nors.push_back({ 0,-1,0 });
		}
		if( genUvs ) {
			uvs.push_back(circleUv);
			uvs.push_back({ sideU,1 });
			uvs.push_back({ sideU,0 });
			uvs.push_back(circleUv);
		}
	}
	const int nrCols = 4;
	for( int i=0; i<nrSlices; i++ )
	{
		tris.push_back({ 0,          2+nrCols*i,     2+nrCols*(i+1) });
		tris.push_back({ 3+nrCols*i, 4+nrCols*i, 	 4+nrCols*(i+1) }); // upper
		tris.push_back({ 3+nrCols*i, 4+nrCols*(i+1), 3+nrCols*(i+1) }); // lower
		tris.push_back({ 1,          5+nrCols*(i+1), 5+nrCols*i 	});
	}
}






MeshCapsule::MeshCapsule(float width, float height, int nrSlices, int nrStacks, bool genNors, bool genUvs)
{
	const float radius = width*0.5f;
	name = fmtStrToBuf("capsule_sl%d_st", nrSlices, nrStacks);

	const float halfCylinder = height/2.f - radius;
	const int halfStacks = nrStacks / 2;
	nrStacks = halfStacks * 2;

	const float fNrSlices = (float)nrSlices;
	const float fNrStacks = (float)nrStacks;

	nr_tris = nrSlices*2 + (nrStacks-2)*nrSlices*2;
	tris.reserve(nr_tris);

	if( genUvs ) // has seams
	{
		nr_verts = 2*nrSlices + 2*(nrSlices+1)*(nrStacks-1);
		poss.reserve(nr_verts);
		uvs.reserve(nr_verts);
		if( genNors )
			nors.reserve(nr_verts);


		bool doneMakeDupLings = false;
		float posShiftHeight = halfCylinder;
		float uvShiftHeight = 0.5f;

		// 1. make inter triangles
		for( int stack = 1; stack < nrStacks; stack++ )
		{
			float phi = glim::pi90 - glim::pi * stack / fNrStacks;
			float y = sin(phi);
			float cosPhi = cos(phi);
			for( int slice = 0; slice <= nrSlices; slice++ )
			{
				float theta = glim::pi2 * slice / fNrSlices;
				float x = cosPhi * cos(theta);
				float z = -cosPhi * sin(theta);
				vec3 nor = {x, y, z};
				vec3 pos = radius * nor;
				vec2 uv = { 1.f*slice/(float)nrSlices, (1.f-stack/(float)(nrStacks-1)) };
				pos.y += posShiftHeight;
				uv.y += uvShiftHeight;

				poss.push_back(pos);
				uvs.push_back(uv);
				if( genNors ) {
					nors.push_back(nor);
				}
			}
			// if half do again
			if( !doneMakeDupLings && stack == halfStacks ) {
				posShiftHeight = -halfCylinder;
				uvShiftHeight = -0.5f;
				stack--;
				doneMakeDupLings = true;
			}
		}
		for( int stack = 0; stack < nrStacks-1; stack++ )
		{
			int curRow = (nrSlices+1)*stack;
			int nextRow = (nrSlices+1)*(stack+1);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
			/*
				cc nc
				cn nn
			*/
				int curCol = slice;
				int nextCol = slice+1;
				tris.push_back({ curCol+curRow, nextCol+nextRow, nextCol+curRow });
				tris.push_back({ curCol+curRow, curCol+nextRow, nextCol+nextRow });
			}
		}


		// 2. make top and bot triangles
		const float halfHeight = height*0.5f;
		int topIdx = poss.size();
		for( int slice = 0; slice < nrSlices; slice++ ) {
			poss.push_back({0, halfHeight, 0});
			uvs.push_back({ 2.f*slice/fNrSlices, 1.f });
			if( genNors )
				nors.push_back({0,1,0});
		}
		int botIdx = poss.size();
		for( int slice = 0; slice < nrSlices; slice++ ) {
			poss.push_back({0, -halfHeight, 0});
			uvs.push_back({ 2.f*slice/fNrSlices, 0.f });
			if( genNors )
				nors.push_back({0,-1,0});
		}

		int topRow = 0;
		int botRow = (nrSlices+1)*(nrStacks-1);
		for( int slice = 0; slice < nrSlices; slice++ ) {
			int curCol = slice;
			int nextCol = slice+1;
			tris.push_back({ curCol+topRow, nextCol+topRow, topIdx+slice });
			tris.push_back({ nextCol+botRow, curCol+botRow, botIdx+slice });
		}
	}
	else // no seams, all shared verts mesh
	{
		nr_verts = (nrSlices)*(nrStacks-1)+2;
		poss.reserve(nr_verts);
		if( genNors ) {
			nors.reserve(nr_verts);
		}

		bool doneMakeDupLings = false;
		float posShiftHeight = halfCylinder;

		// 1. make inter triangles
		for( int stack = 1; stack < nrStacks; stack++ )
		{
			float phi = glim::pi90 - glim::pi * stack / fNrStacks;
			float y = sin(phi);
			float cosPhi = cos(phi);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
				float theta = glim::pi2 * slice / fNrSlices;
				float x = cosPhi * cos(theta);
				float z = -cosPhi * sin(theta);
				vec3 nor = {x, y, z};
				vec3 pos = radius * nor;
				pos.y += posShiftHeight;
				poss.push_back(pos);
				if( genNors ) {
					nors.push_back(nor);
				}
			}
			// if half do again
			if( !doneMakeDupLings && stack == halfStacks ) {
				posShiftHeight = -halfCylinder;
				stack--;
				doneMakeDupLings = true;
			}
		}
		for( int stack = 0; stack < nrStacks-1; stack++ )
		{
			int curRow = nrSlices*stack;
			int nextRow = nrSlices*(stack+1);
			for( int slice = 0; slice < nrSlices; slice++ )
			{
			/*
				cc nc
				cn nn
			*/
				int curCol = slice;
				int nextCol = (slice==nrSlices-1) ? 0 : slice+1;
				tris.push_back({ curCol+curRow, nextCol+nextRow, nextCol+curRow });
				tris.push_back({ curCol+curRow, curCol+nextRow, nextCol+nextRow });
			}
		}
		

		// 2. make top and bot triangles
		int topIdx = poss.size();
		int botIdx = topIdx + 1;
		poss.push_back({0, height*0.5f, 0});
		poss.push_back({0, -height*0.5f, 0});
		if( genNors ) {
			nors.push_back({0, 1,0});
			nors.push_back({0,-1,0});
		}

		int topRow = 0;
		int botRow = nrSlices*(nrStacks-1);
		for( int slice = 0; slice < nrSlices; slice++ ) {
			int curCol = slice;
			int nextCol = (slice==nrSlices-1) ? 0 : slice+1;
			tris.push_back({ curCol+topRow, nextCol+topRow, topIdx });
			tris.push_back({ nextCol+botRow, curCol+botRow, botIdx });
		}
	}







	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	// for( int stack=0; stack<nrStacks; stack++ )
	// {
	// 	float phi = glim::pi90 - glim::pi * stack/(float)(nrStacks-1);
	// 	float y = sin(phi);
	// 	float cosPhi = cos(phi);
	// 	for( int slice=0; slice<=nrSlices; slice++ )
	// 	{
	// 		float theta = glim::pi2 * slice / (float)nrSlices;
	// 		float x = cosPhi * cos(theta);
	// 		float z = -cosPhi * sin(theta);
	// 		vec3 nor = {x, y, z};
	// 		vec3 pos = radius * nor;
	// 		vec2 uv = { 1.f*slice/(float)nrSlices, (1.f-stack/(float)(nrStacks-1)) };
	// 		pos.y += (stack<halfStacks)? halfCylinder : (-halfCylinder);
	// 		uv.y += (stack<halfStacks)? 0.5f : -0.5f;
	// 		uv.y = fract(uv.y);

	// 		poss.push_back(pos);
	// 		if( genNors ) nors.push_back(nor);
	// 		if( genUvs )  uvs.push_back(uv);
	// 	}
	// }

	// const int nrCols = nrSlices + 1;

	// for (int stack = 0; stack < nrStacks; stack++)
	// {
	// 	int curRow = nrCols * stack;
	// 	int nextRow = nrCols * (stack + 1);
	// 	for (int slice = 0; slice < nrSlices; slice++)
	// 	{
	// 		int cur_col = slice;
	// 		int next_col = slice + 1;
	// 		if (stack < nrStacks) { // upper
	// 			tris.push_back({ curRow+cur_col, nextRow+cur_col, nextRow+next_col });
	// 		}
	// 		if (stack > 0) { // lower
	// 			tris.push_back({ curRow+cur_col, nextRow+next_col, curRow+next_col });
	// 		}
	// 	}
	// }
}






MeshDonut::MeshDonut(float width, float height, int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("donut_s%d_r", nrSlices, nrRingVerts);

	
	const float ringRad = height*0.5f;
	const float donutRad = width*0.5f;

	// calculus : shell method
	for( int slice=0; slice<=nrSlices; slice++ )
	{
		float donutTheta = -glim::pi2*slice/(float)nrSlices;
		for( int rv=0; rv<=nrRingVerts; rv++ )
		{
			float ringTheta = glim::pi + glim::pi2*rv/(float)nrRingVerts;
			float y = ringRad * sin(ringTheta);
			float relativeX = donutRad + ringRad * cos(ringTheta);
			float x = relativeX * cos(donutTheta);
			float z = relativeX * sin(donutTheta);

			poss.push_back({ x,y,z });
			if( genNors ) nors.push_back( normalize(poss.back()));
			if( genUvs )  uvs.push_back({ 2.f*slice/(float)nrSlices, rv/(float)nrRingVerts });
		}
	}

	int nrRealRingVerts = nrRingVerts+1;
	for( int slice=0; slice<nrSlices; slice++ )
	{
		int curRing = nrRealRingVerts * slice;
		int nextRing = curRing + nrRealRingVerts;
		for( int rv=0; rv<nrRingVerts; rv++ )
		{
			int curVert = rv;
			int nextVert = rv + 1;

			tris.push_back({ curRing+curVert, nextRing+curVert, nextRing+nextVert }); // upper
			tris.push_back({ curRing+curVert, nextRing+nextVert, curRing+nextVert }); // lower
		}
	}
}

MeshDonutWithRadius::MeshDonutWithRadius(float outerRadius, float innerRadius, int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
	: MeshDonut::MeshDonut(outerRadius, outerRadius-innerRadius, nrSlices, nrRingVerts, genNors, genUvs)
{
}

MeshDonutWithRadius2::MeshDonutWithRadius2(float donutRadius, float thickRadius, int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
	: MeshDonut::MeshDonut(donutRadius+thickRadius, thickRadius, nrSlices, nrRingVerts, genNors, genUvs)
{
}