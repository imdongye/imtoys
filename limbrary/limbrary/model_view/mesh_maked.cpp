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
*/

#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/texture.h>
#include <limbrary/utils.h>
#include <vector>
#include <memory>
#include <glad/glad.h>

using namespace lim;
using namespace glm;


MeshQuad::MeshQuad(bool genNors, bool genUvs)
{
	name = "quad";

	const float half = 1.0f;
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

MeshPlane::MeshPlane(int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("plane_s%d", nrSlices);
	
	const float length = 2.0f;
	const float start = -length / 2.f;
	const float step = length / nrSlices;
	const vec3 up = {0, 1, 0};
	const float div = nrSlices;


	for(int i = 0; i <= nrSlices; i++) for(int j = 0; j <= nrSlices; j++)
	{
		poss.push_back({start + step * j, 0, start + step * i});
		if( genNors ) {
			nors.push_back( up );
		}
		if( genUvs ) {
			uvs.push_back({ j/div, (div-i)/div });
		}
	}


	const int nrCols = nrSlices + 1;
	for (int i = 0; i < nrSlices; i++) for (int j = 0; j < nrSlices; j++)
	{
		// 0-1
		// |\|
		// 2-3
		const int ori = i * nrCols + j;

		// lower
		tris.push_back({ori + 0,
						ori + 0 + nrCols,
						ori + 1 + nrCols});
		// upper
		tris.push_back({ori + 0,
						ori + 1 + nrCols,
						ori + 1});
	}
	
	initGL();
}

MeshCube::MeshCube(bool genNors, bool genUvs)
{
	name = "cube";
	
	const float half = 1.0f;
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
		tris.push_back({ 0+i*4, 1+i*4, 2+i*4 });
		tris.push_back({ 0+i*4, 2+i*4, 3+i*4 });
	}

	initGL();
}

// From: http://www.songho.ca/opengl/gl_sphere.html
// texture coord가 다른 같은 위치의 vertex가 많음
MeshSphere::MeshSphere(int nrSlices, int nrStacks, bool genNors, bool genUvs, float radius)
{
	name = fmtStrToBuf("sphere_sl%d_st%d", nrSlices, nrStacks);

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	for (int stack = 0; stack <= nrStacks; stack++)
	{
		float phi = H_PI - F_PI * stack / (float)nrStacks;
		float y = radius* sin(phi);
		float r_cosPhi = radius * cos(phi);
		for (int slice = 0; slice <= nrSlices; slice++)
		{
			float theta = D_PI * slice / (float)nrSlices;
			float x = r_cosPhi * cos(theta);
			float z = -r_cosPhi * sin(theta);
			vec3 pos = {x, y, z};
			poss.push_back(pos);
			if( genNors ) {
				nors.push_back(normalize(pos));
			}
			if( genUvs ) {
				uvs.push_back({ 2.f*slice/(float)nrSlices, 1.f - stack/(float)nrStacks });
			}
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
				tris.push_back({ curRow+cur_col, nextRow+cur_col, nextRow+next_col });
			}
			if( stack > 0 ) { // lower
				tris.push_back({ curRow+cur_col, nextRow+next_col, curRow+next_col });
			}
		}
	}

	initGL();
}
MeshEnvSphere::MeshEnvSphere(int nrSlices, int nrStacks)
{
	name = fmtStrToBuf("env_sphere_sl%d_st%d", nrSlices, nrStacks);
	
	const float radius = 1.f;

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	for (int stack = 0; stack <= nrStacks; stack++)
	{
		float phi = H_PI - F_PI * stack / (float)nrStacks;
		float y = sin(phi);
		float r_cosPhi = radius * cos(phi);
		for (int slice = 0; slice <= nrSlices; slice++)
		{
			float theta = D_PI * slice / (float)nrSlices;
			float x = r_cosPhi * cos(theta);
			float z = r_cosPhi * sin(theta);
			vec3 pos = {x, y, z};
			poss.push_back(pos);
			nors.push_back(normalize(-pos));
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
MeshIcoSphere::MeshIcoSphere(int subdivision, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("icosphere_s%d", subdivision);
	
	
	const float uStep = 1.f / 11.f;
	const float vStep = 1.f / 3.f;
	const float radius = 1.f;

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

	const float aStep = D_PI / 5.f;		 // angle step
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

MeshCubeSphere::MeshCubeSphere(int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("cubesphere_s%d", nrSlices);

	const float radius = 1.f;

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
MeshCubeSphere2::MeshCubeSphere2(int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("smoothcubesphere_s%d", nrSlices);
	
	
	const float radius = 1.f;
	const vec3 cbNors[6] = {
		{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
	const vec3 cbTans[6] = {
		{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

	/* genUnitCubeSpherePositiveXFace */
	std::vector<vec3> facePoints;

	for( int y=0; y<=nrSlices; y++ ) { // z-axis angle
		float phi = H_PI * y/(float)nrSlices - Q_PI;
		vec3 n1 = { -sin(phi), cos(phi), 0 };
		for( int x=0; x<=nrSlices; x++ ) { // y-axis angle
			float theta = H_PI * (float)x / nrSlices - Q_PI;
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

MeshCylinder::MeshCylinder(int nrSlices, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("sylinder_s%d", nrSlices);
	
	const float radius = 1.f;
	const float half = 1.f; // height

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
		float theta = D_PI*i/(float)nrSlices;
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

MeshCapsule::MeshCapsule(int nrSlices, int nrStacks, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("capsule_sl%d_st", nrSlices, nrStacks);

	const float radius = 1.f;
	const float halfSylinder = 1.f; // height
	const int halfStacks = nrStacks / 2;
	nrStacks = halfStacks * 2;

	// phi : angle form xy-plane [-pi/2, pi/2]
	// theta : y-axis angle [0, 2pi]
	for( int stack=0; stack<nrStacks; stack++ )
	{
		float phi = H_PI - F_PI * stack/(float)(nrStacks-1);
		float y = sin(phi);
		float r_cosPhi = radius * cos(phi);
		for( int slice=0; slice<=nrSlices; slice++ )
		{
			float theta = D_PI * slice / (float)nrSlices;
			float x = r_cosPhi * cos(theta);
			float z = -r_cosPhi * sin(theta);
			vec3 pos = {x, y, z};
			vec3 nor = normalize(pos);
			vec2 uv = { 1.f*slice/(float)nrSlices, (1.f-stack/(float)(nrStacks-1)) };
			pos.y += (stack<halfStacks)? halfSylinder : (-halfSylinder);
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

MeshDonut::MeshDonut(int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
{
	name = fmtStrToBuf("donut_s%d_r", nrSlices, nrRingVerts);

	
	const float ringRad = 1.f;
	const float donutRad = 1.5f;

	// calculus : shell method
	for( int slice=0; slice<=nrSlices; slice++ )
	{
		float donutTheta = -D_PI*slice/(float)nrSlices;
		for( int rv=0; rv<=nrRingVerts; rv++ )
		{
			float ringTheta = F_PI + D_PI*rv/(float)nrRingVerts;
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
