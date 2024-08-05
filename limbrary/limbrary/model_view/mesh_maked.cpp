/*
	for generate mesh of general shape
 	2023-01-17 / im dong ye

	uv : upper left
	st : down left

	Note:
	texture repeat가정

	todo :
	1. bumpmap normalmap확인
	2. https://modoocode.com/129
	3. 최소 최대 slice 예외처리
	4. reserve vector
	5. subdivision with shared vertex
	6. make separate vertex, normal with triangles
*/

#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/texture.h>
#include <limbrary/utils.h>
#include <vector>
#include <memory>
#include <glad/glad.h>
#include <limbrary/glm_tools.h>

using namespace lim;
using namespace glm;


MeshQuad::MeshQuad(float width, bool genNors, bool genUvs)
{
	name = "quad";

	const float half = width/2.f;
	const vec3 front = {0, 0, 1};

	poss.push_back({-half, half, 0});
	poss.push_back({half, half, 0});
	poss.push_back({-half, -half, 0});
	poss.push_back({half, -half, 0});

	if( genNors ) {
		nors.push_back(front);
		nors.push_back(front);
		nors.push_back(front);
		nors.push_back(front);
	}
	
	if( genUvs ) {
		uvs.push_back({0,1});
		uvs.push_back({1,1});
		uvs.push_back({0,0});
		uvs.push_back({1,0});
	}

	tris.push_back({0,3,1});
	tris.push_back({0,2,3});

	initGL();
}

MeshPlane::MeshPlane(float width, int nrCols, int nrRows, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("plane_%d_%d", nrCols, nrRows);
	const float fNrCols = nrCols;
	const float fNrRows = nrRows;
	const float start = -width / 2.f;
	const float stepX = width / fNrCols;
	const float stepY = width / fNrRows;
	const vec3 up = {0, 1, 0};


	for(int i = 0; i <= nrRows; i++) for(int j = 0; j <= nrCols; j++)
	{
		poss.push_back({start + stepX * j, 0, start + stepY * i});
		if( genNors ) {
			nors.push_back( up );
		}
		if( genUvs ) {
			uvs.push_back({ j/fNrCols, (fNrRows-i)/fNrRows });
		}
	}

	const int nrColVerts = nrCols+1;
	for (int i = 0; i < nrRows; i++) for (int j = 0; j < nrCols; j++)
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
	
	initGL();
}

MeshCube::MeshCube(float width, bool genNors, bool genUvs)
{
	name = "cube";
	
	const float half = width/2.f;
	const vec3 cbNors[6] = {
		{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
	const vec3 cbTans[6] = {
		{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

	for (int i = 0; i < 6; i++)
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

	for (unsigned int i = 0; i < 6; i++)
	{
		tris.push_back({ 0+i*4, 1+i*4, 2+i*4 });
		tris.push_back({ 0+i*4, 2+i*4, 3+i*4 });
	}

	initGL();
}




// From: http://www.songho.ca/opengl/gl_sphere.html
// texture coord가 다른, 같은 위치의 vertex가 많음
// Todo: 양끝 버텍스 두개 남는거 고치기
MeshSphere::MeshSphere(float width, int nrSlices, int nrStacks, bool genNors, bool genUvs)
{
	const float radius = width/2.f;
	name = fmtStrToBuf("sphere_sl%d_st%d", nrSlices, nrStacks);

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]

	vec3 nor = {0, -1, 0};
	for( int slice = 0; slice < nrSlices; slice++ ) {
		poss.push_back(nor*radius);
		if( genNors ) nors.push_back(nor);
		if( genUvs )  uvs.push_back({ 2.f*slice/(float)nrSlices, 0.f });
	}

	nor = {0, 1, 0};
	for( int slice = 0; slice < nrSlices; slice++ ) {
		poss.push_back(nor*radius);
		if( genNors ) nors.push_back(nor);
		if( genUvs )  uvs.push_back({ 2.f*slice/(float)nrSlices, 1.f });
	}

	for( int stack = 1; stack < nrStacks; stack++ )
	{
		float phi = glim::pi90 - glim::pi * stack / (float)nrStacks;
		float y = sin(phi);
		float cosPhi = cos(phi);
		for (int slice = 0; slice <= nrSlices; slice++)
		{
			float theta = glim::pi2 * slice / (float)nrSlices;
			float x = cosPhi * cos(theta);
			float z = -cosPhi * sin(theta);
			nor = {x, y, z};
			poss.push_back(nor*radius);
			if( genNors ) {
				nors.push_back(nor);
			}
			if( genUvs ) {
				uvs.push_back({ 2.f*slice/(float)nrSlices, 1.f - stack/(float)nrStacks });
			}
		}
	}
	

	const int nrCols = nrSlices + 1;

	for (int stack = 0; stack < nrStacks; stack++)
	{
		int curRow = (stack)? nrSlices+nrCols*(stack-1) : 0;
		int nextRow = nrCols * (stack + 1);
		for (int slice = 0; slice < nrSlices; slice++)
		{
			int cur_col = slice;
			int next_col = slice + 1;
			if( stack < nrStacks ) { // upper
				tris.push_back({ curRow+cur_col, nextRow+cur_col, nextRow+next_col });
			}
			if( stack > 0 ) { // lower
				tris.push_back({ curRow+cur_col, nextRow+next_col, curRow+next_col });
			}
		}
	}

	initGL();
}
MeshEnvSphere::MeshEnvSphere(float width, int nrSlices, int nrStacks)
{
	const float radius = width/2.f;
	name = fmtStrToBuf("env_sphere_sl%d_st%d", nrSlices, nrStacks);

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	for (int stack = 0; stack <= nrStacks; stack++)
	{
		float phi = glim::pi90 - glim::pi * stack / (float)nrStacks;
		float y = sin(phi);
		float cosPhi = cos(phi);
		for (int slice = 0; slice <= nrSlices; slice++)
		{
			float theta = glim::pi2 * slice / (float)nrSlices;
			float x = cosPhi * cos(theta);
			float z = cosPhi * sin(theta);
			vec3 nor = {x, y, z};
			poss.push_back(nor*radius);
			nors.push_back(nor);
			uvs.push_back({ slice/(float)nrSlices, 1.f - stack/(float)nrStacks });
		}
	}

	const int nrCols = nrSlices + 1;

	for (int stack = 0; stack < nrStacks; stack++)
	{
		int curRow = nrCols * stack;
		int nextRow = nrCols * (stack + 1);
		for (int slice = 0; slice < nrSlices; slice++)
		{
			int cur_col = slice;
			int next_col = slice + 1;
			if( stack < nrStacks ){ // upper
				tris.push_back({ curRow+cur_col, nextRow+next_col, nextRow+cur_col });
			}
			if( stack > 0 ) { // lower
				tris.push_back({ curRow+cur_col, curRow+next_col, nextRow+next_col });
			}
		}
	}

	initGL();
}

// icosahedron, 20면체
MeshIcoSphere::MeshIcoSphere(float width, int subdivision, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("icosphere_s%d", subdivision);
	
	
	const float uStep = 1.f / 11.f;
	const float vStep = 1.f / 3.f;
	const float radius = width/2.f;

	// 위 아래 꼭지점
	for (int i = 0; i < 5; i++)
	{
		poss.push_back({0, radius,0});
		poss.push_back({0,-radius,0});

		if( genNors ) {
			nors.push_back({0, 1,0});
			nors.push_back({0,-1,0});
		}
		if( genUvs ) {
			uvs.push_back({ uStep*(1+ 2*i), 1 });
			uvs.push_back({ uStep*(3+ 2*i), 0 });
		}
	}

	const float aStep = glim::pi2 / 5.f;		 // angle step
	const float halfH = radius * 0.5f;	 // half height
	const float base = halfH * sqrtf(3); // bottom length 밑변

	float topA = 0; // top angle
	float botA = aStep * 0.5f;

	// 옆 부분
	for (int i = 0; i < 6; i++)
	{
		poss.push_back({ base*cos(topA), halfH, -base*sin(topA) });
		if(genNors) nors.push_back( normalize(poss.back()) );
		if(genUvs) 	uvs.push_back({ uStep*(2*i), 2*vStep });
		topA += aStep;

		poss.push_back({ base*cos(botA), -halfH, -base*sin(botA)});
		if(genNors)	nors.push_back( normalize(poss.back()) );
		if(genUvs) 	uvs.push_back({ uStep*(1+2*i), vStep });
		botA += aStep;
	}
	tris.push_back({0,  10, 12});
	tris.push_back({2,  12, 14});
	tris.push_back({4,  14, 16});
	tris.push_back({6,  16, 18});
	tris.push_back({8,  18, 20});

	tris.push_back({10, 11, 12});
	tris.push_back({12, 13, 14});
	tris.push_back({14, 15, 16});
	tris.push_back({16, 17, 18});
	tris.push_back({18, 19, 20});

	tris.push_back({13, 12, 11});
	tris.push_back({15, 14, 13});
	tris.push_back({17, 16, 15});
	tris.push_back({19, 18, 17});
	tris.push_back({21, 20, 19});

	tris.push_back({1,  13, 11});
	tris.push_back({3,  15, 13});
	tris.push_back({5,  17, 15});
	tris.push_back({7,  19, 17});
	tris.push_back({9,  21, 19});

	// Todo : 중복되는 버텍스 없애기
	for( int i=0; i<subdivision; i++ )
	{
		std::vector<uvec3> copiedTris = std::move(tris); // Todo: move속도비교해보기
		tris = std::vector<uvec3>();

		for( const uvec3& srcTri : copiedTris )
		{
			const GLuint i1 = srcTri.x;
			const GLuint i2 = srcTri.y;
			const GLuint i3 = srcTri.z;
			const GLuint newIdx = poss.size();

			// Todo: MSVC에서 레퍼런스 배열 -1.98 값으로 버그
			// const vec3& p1 = poss[i1];
			// const vec3& p2 = poss[i2];
			// const vec3& p3 = poss[i3];
			// poss.push_back( normalize( ( p1+p2 )*0.5f) );
			// poss.push_back( normalize( ( p2+p3 )*0.5f) );
			// poss.push_back( normalize( ( p3+p1 )*0.5f) );

			poss.push_back( normalize( ( poss[i1]+poss[i2] )*0.5f) );
			poss.push_back( normalize( ( poss[i2]+poss[i3] )*0.5f) );
			poss.push_back( normalize( ( poss[i3]+poss[i1] )*0.5f) );

			if( genNors ) {
				nors.push_back( poss[newIdx+0] );
				nors.push_back( poss[newIdx+1] );
				nors.push_back( poss[newIdx+2] );
			}
			poss[newIdx+0] *= radius;
			poss[newIdx+1] *= radius;
			poss[newIdx+2] *= radius;

			if( genUvs ) {
				uvs.push_back( (uvs[i1]+uvs[i2])*0.5f );
				uvs.push_back( (uvs[i2]+uvs[i3])*0.5f );
				uvs.push_back( (uvs[i3]+uvs[i1])*0.5f );
			}

			tris.push_back({ i1, newIdx+0, newIdx+2 });
			tris.push_back({ i2, newIdx+1, newIdx+0 });
			tris.push_back({ i3, newIdx+2, newIdx+1 });
			tris.push_back({ newIdx+0,  newIdx+1, newIdx+2 });
		}
	}

	initGL();
}

MeshCubeSphere::MeshCubeSphere(float width, int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("cubesphere_s%d", nrSlices);

	const float radius = width/2.f;

	const vec3 cbNors[6] = {
		{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
	const vec3 cbTans[6] = {
		{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

	for (int side = 0; side < 6; side++)
	{
		vec3 n = cbNors[side];
		vec3 t = cbTans[side];
		vec3 b = cross(n, t);

		GLuint offset = poss.size();

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
			const GLuint curRow = offset + y * nrCols;
			const GLuint nextRow = offset + (y + 1) * nrCols;
			for (int x = 0; x < nrSlices; x++)
			{
				tris.push_back({ nextRow+x, curRow+x, 	curRow+x+1  });
				tris.push_back({ nextRow+x, curRow+x+1, nextRow+x+1 });
			}
		}
	}

	initGL();
}

// smooth
MeshCubeSphere2::MeshCubeSphere2(float width, int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("smoothcubesphere_s%d", nrSlices);
	
	
	const float radius = width/2.f;
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
			const GLuint curRow = offset + y*nrCols;
			const GLuint upRow = offset + (y+1)*nrCols;
			for( int x=0; x<nrSlices; x++ ) {
				tris.push_back({upRow+x, curRow+x, curRow+x+1});
				tris.push_back({upRow+x, curRow+x+1, upRow+x+1});
			}
		}
	}
	
	initGL();
}

MeshCylinder::MeshCylinder(float width, float height, int nrSlices, bool genNors, bool genUvs)
{
	const float radius = width/2.f;
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

	for( GLuint i=0; i<=nrSlices; i++ )
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
	const GLuint nrCols = 4;
	for( GLuint i=0; i<nrSlices; i++ )
	{
		tris.push_back({ 0,          2+nrCols*i,     2+nrCols*(i+1) });
		tris.push_back({ 3+nrCols*i, 4+nrCols*i, 	 4+nrCols*(i+1) }); // upper
		tris.push_back({ 3+nrCols*i, 4+nrCols*(i+1), 3+nrCols*(i+1) }); // lower
		tris.push_back({ 1,          5+nrCols*(i+1), 5+nrCols*i 	});
	}

	initGL();
}

MeshCapsule::MeshCapsule(float width, float height, int nrSlices, int nrStacks, bool genNors, bool genUvs)
{
	const float radius = width/2.f;
	name = fmtStrToBuf("capsule_sl%d_st", nrSlices, nrStacks);

	const float halfCylinder = height/2.f - radius;
	const int halfStacks = nrStacks / 2;
	nrStacks = halfStacks * 2;

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	for( int stack=0; stack<nrStacks; stack++ )
	{
		float phi = glim::pi90 - glim::pi * stack/(float)(nrStacks-1);
		float y = sin(phi);
		float cosPhi = cos(phi);
		for( int slice=0; slice<=nrSlices; slice++ )
		{
			float theta = glim::pi2 * slice / (float)nrSlices;
			float x = cosPhi * cos(theta);
			float z = -cosPhi * sin(theta);
			vec3 nor = {x, y, z};
			vec3 pos = radius * nor;
			vec2 uv = { 1.f*slice/(float)nrSlices, (1.f-stack/(float)(nrStacks-1)) };
			pos.y += (stack<halfStacks)? halfCylinder : (-halfCylinder);
			uv.y += (stack<halfStacks)? 0.5f : -0.5f;
			uv.y = fract(uv.y);

			poss.push_back(pos);
			if( genNors ) nors.push_back(nor);
			if( genUvs )  uvs.push_back(uv);
		}
	}

	const int nrCols = nrSlices + 1;

	for (int stack = 0; stack < nrStacks; stack++)
	{
		int curRow = nrCols * stack;
		int nextRow = nrCols * (stack + 1);
		for (int slice = 0; slice < nrSlices; slice++)
		{
			int cur_col = slice;
			int next_col = slice + 1;
			if (stack < nrStacks) { // upper
				tris.push_back({ curRow+cur_col, nextRow+cur_col, nextRow+next_col });
			}
			if (stack > 0) { // lower
				tris.push_back({ curRow+cur_col, nextRow+next_col, curRow+next_col });
			}
		}
	}

	initGL();
}

MeshDonut::MeshDonut(float radius, float height, int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("donut_s%d_r", nrSlices, nrRingVerts);

	
	const float ringRad = height/2.f;
	const float donutRad = radius;

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

	initGL();
}

/*
	w z
	x y 
*/
static void addTriFace(uvec4 quad, std::vector<uvec3>& tris) {
	tris.emplace_back(quad.x, quad.y, quad.z);
	tris.emplace_back(quad.x, quad.z, quad.w);
}

MeshCubeShared::MeshCubeShared(float width, bool withInitGL)
{
	name = "shared cube";
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
	addTriFace({0,1,3,2}, tris);
	addTriFace({6,7,5,4}, tris);
	addTriFace({2,3,7,6}, tris);
	addTriFace({3,1,5,7}, tris);
	addTriFace({1,0,4,5}, tris);
	addTriFace({0,2,6,4}, tris);
	
	if( withInitGL ) {
		initGL();
	}
}


MeshCloth::MeshCloth(vec2 size, float innerWidth) {
	ivec2 nrSlice = { int(size.x/innerWidth), int(size.y/innerWidth) };
	vec2 innerSize = size/vec2(nrSlice);
	int nrPoss = (nrSlice.x+1) * (nrSlice.y+1);
	vec2 sPos = -size/2.f;

	name = fmtStrToBuf("cloth_%d_%d", nrSlice.x, nrSlice.y);
	const vec3 up = {0, 1, 0};

	poss.reserve(nrPoss);
	nors.reserve(nrPoss);
	uvs.reserve(nrPoss);
	tris.reserve(2*nrSlice.x*nrSlice.y);
	for( int i=0; i<=nrSlice.y; i++ ) for( int j=0; j<=nrSlice.x; j++ )
	{
		poss.push_back({sPos.x + innerSize.x*j, 0, sPos.y + innerSize.y*i});
		nors.push_back( up );
		uvs.push_back({ j/float(nrSlice.x), (nrSlice.y-i)/float(nrSlice.x) });
	}

	const int szRow = nrSlice.x+1;
	for( int i=0; i<nrSlice.y; i++ ) for( int j=0; j<nrSlice.x; j++ )
	{
		const int ori = j + szRow*i;
		// 0-1
		// |\|
		// 2-3
		if( (i+j)%2==0 ) {
			// upper
			tris.push_back({ori + 0,
							ori + 1 + szRow,
							ori + 1});
			// lower
			tris.push_back({ori + 0,
							ori + 0 + szRow,
							ori + 1 + szRow});
		}
		// 0-1
		// |/|
		// 2-3
		else {
			// upper
			tris.push_back({ori + 0,
							ori + 0 + szRow,
							ori + 1});
			// lower
			tris.push_back({ori + 1,
							ori + 0 + szRow,
							ori + 1 + szRow});
		}
	}
	initGL();
}



MeshSphereShared::MeshSphereShared(float width, int nrSlices, int nrStacks, bool withInitGL)
{
	name = "shared sphere";

	const float radius = width*0.5f;
	const int nrRows = nrStacks+1;
	const int nrVerts = 2+(nrRows-2)*nrSlices;
	const int nrTris = 2*nrSlices + (nrRows-2)*nrSlices*2;

	tris.reserve(nrTris);
	poss.reserve(nrVerts);
	poss.push_back({0,-radius,0});
	poss.push_back({0, radius,0});
	int i, j;
	for( i=1; i<nrStacks; i++ )
	{
		float phi = -glim::pi90 + glim::pi*i/(float)nrStacks;
		float y = sin(phi);
		float cosPhi = cos(phi);
		for( j=0; j<nrSlices; j++ )
		{
			float theta = glim::pi2 * j / (float)nrSlices;
			float x = cosPhi * cos(theta);
			float z = cosPhi * sin(theta);
			vec3 nor = {x, y, z};
			poss.push_back(nor*radius);
		}
	}

	int offsetTop = 2 + nrSlices*(nrStacks-2);
	for( i=0; i<nrSlices-1; i++ )
	{
		tris.push_back({0, 3+i, 2+i});
		tris.push_back({1, offsetTop+i, 1+offsetTop+i});
	}
	tris.push_back({0, 2, 2+i});
	tris.push_back({1, offsetTop+i, offsetTop});


	for( i=0; i<nrRows-3; i++ ) {
		int offL = 2+nrSlices*i;
		int offH = 2+nrSlices*(i+1);
		for( j=0; j<nrSlices-1; j++ ) {
			tris.push_back({offL+j, offH+j+1, offH+j});
			tris.push_back({offL+j, offL+j+1, offH+j+1});
		}
		tris.push_back({offL+j, offH, offH+j});
		tris.push_back({offL+j, offL, offH});
	}

	if( withInitGL )
		initGL();
}