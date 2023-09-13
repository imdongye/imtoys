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
// 	3. 최소 최대 slice 예외처리
//

#include <limbrary/model_view/code_mesh.h>
#include <limbrary/texture.h>
#include <limbrary/utils.h>
#include <vector>
#include <memory>
#include <glad/glad.h>

using namespace lim;
using namespace glm;

namespace lim::code_mesh
{
	Mesh* genQuad(bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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

		rst->initGL();
		return rst;
	}

	Mesh* genPlane(int nrSlice, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

		const float length = 2.0f;
		const float start = -length / 2.f;
		const float step = length / nrSlice;
		const vec3 up = {0, 1, 0};
		const float div = nrSlice + 1;


		for(int i = 0; i <= nrSlice; i++) for(int j = 0; j <= nrSlice; j++)
		{
			poss.push_back({start + step * j, 0, start + step * i});
			if( genNors ) {
				nors.push_back( up );
			}
			if( genUvs ) {
				uvs.push_back({ j/div, (div-i)/div });
			}
		}


		const int nrCols = nrSlice + 1;
		for (int i = 0; i < nrSlice; i++) for (int j = 0; j < nrSlice; j++)
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
		
		rst->initGL();
		return rst;
	}

	Mesh* genCube(bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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

		rst->initGL();
		return rst;
	}

	// From: http://www.songho.ca/opengl/gl_sphere.html
	// texture coord가 다른 같은 위치의 vertex가 많음
	Mesh* genSphere(const int nrSlices, const int nrStacks, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

		const float radius = 1.f;

		// phi : angle form xy-plane [-pi/2, pi/2]
		// theta : y-axis angle [0, 2pi]
		for (int stack = 0; stack <= nrStacks; stack++)
		{
			float phi = H_PI - F_PI * stack / (float)nrStacks;
			float y = sin(phi);
			float rcos = radius * cos(phi);
			for (int slice = 0; slice <= nrSlices; slice++)
			{
				float theta = D_PI * slice / (float)nrSlices;
				float x = rcos * cos(theta);
				float z = -rcos * sin(theta);
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

		rst->initGL();
		return rst;
	}

	// icosahedron, 20면체
	Mesh* genIcoSphere(const int subdivision, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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

		vec3 pos, norm;
		vec2 uv;
		const float aStep = D_PI / 5.f;		 // angle step
		const float halfH = radius * 0.5f;	 // half height
		const float base = halfH * sqrtf(3); // bottom length 밑변

		float topA = 0; // top angle
		float botA = aStep * 0.5f;

		// 옆 부분
		for (int i = 0; i < 6; i++)
		{
			poss.push_back({ base*cos(topA), halfH, -base*sin(topA) });
			if(genNors) nors.push_back( normalize(pos) );
			if(genUvs) 	uvs.push_back({ uStep*(2*i), 2*vStep });
			topA += aStep;

			poss.push_back({ base*cos(botA), -halfH, -base*sin(botA)});
			if(genNors)	nors.push_back( normalize(pos) );
			if(genUvs) 	uvs.push_back({ uStep*(1+2*i), vStep });
			botA += aStep;
		}
		tris.push_back({0, 10, 12});
		tris.push_back({2, 12, 14});
		tris.push_back({4, 14, 16});
		tris.push_back({6, 16, 18});
		tris.push_back({8, 18, 20});

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

		tris.push_back({1, 13, 11});
		tris.push_back({3, 15, 13});
		tris.push_back({5, 17, 15});
		tris.push_back({7, 19, 17});
		tris.push_back({9, 21, 19});

		for( int i=0; i<subdivision; i++ )
		{
			std::vector<uvec3> copiedTris = tris;
			tris.clear();
			for( int j=0; j<copiedTris.size(); j++ )
			{
				const vec3& p1 = poss[copiedTris[j].x];
				const vec3& p2 = poss[copiedTris[j].y];
				const vec3& p3 = poss[copiedTris[j].z];

				const GLuint newIdx = poss.size();

				vec3 normedPos[3];
				poss.push_back( normalize((p1+p2)*0.5f) );
				poss.push_back( normalize((p2+p3)*0.5f) );
				poss.push_back( normalize((p3+p1)*0.5f) );

				if( genNors ) {
					nors.push_back( poss[poss.size()-3]);
					nors.push_back( poss[poss.size()-2]);
					nors.push_back( poss[poss.size()-1]);
				}
				poss[poss.size()-3] *= radius;
				poss[poss.size()-2]	*= radius;
				poss[poss.size()-1]	*= radius;

				if( genUvs ) {
					const vec2& uv1 = uvs[copiedTris[j].x];
					const vec2& uv2 = uvs[copiedTris[j].y];
					const vec2& uv3 = uvs[copiedTris[j].z];
					uvs.push_back( (uv1+uv2)*0.5f );
					uvs.push_back( (uv2+uv3)*0.5f );
					uvs.push_back( (uv3+uv1)*0.5f );
				}

				tris.push_back({ copiedTris[j+0], newIdx+0, newIdx+0 });
				tris.push_back({ copiedTris[j+1], newIdx+1, newIdx+1 });
				tris.push_back({ copiedTris[j+2], newIdx+2, newIdx+2 });
				tris.push_back({ newIdx+0,  newIdx+1, newIdx+2 });
			}
		}

		rst->initGL();
		return rst;
	}

	Mesh* genCubeSphere(const int nrSlices, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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

		rst->initGL();
		return rst;
	}

	// smooth
	Mesh* genCubeSphere2(const int nrSlices, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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
			const mat3 rotMat = mat3(cbNors[side], cbTans[side], cross(cbTans[side], cbTans[side]));

			for( int y=0; y<=nrSlices; y++ ) for(int x=0; x<=nrSlices; x++ ) {
				vec3 nor = rotMat * facePoints[nrCols * y + x];
				poss.push_back(nor*radius);
				if( genNors ) nors.push_back(nor);
				if( genUvs )  uvs.push_back({ x/(float)nrSlices, y/(float)nrSlices });
			}

			for( int y=0; y<=nrSlices; y++ ) {
				const GLuint curRow = offset + y*nrCols;
				const GLuint upRow = offset + (y+1)*nrCols;
				for( int x=0; x<nrSlices; x++ ) {
					tris.push_back({upRow+x, curRow+x, curRow+x+1});
					tris.push_back({upRow+x, curRow+x+1, upRow+x+1});
				}
			}
		}
		rst->initGL();
		return rst;
	}

	Mesh* genCylinder(const int nrSlices, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

		const float radius = 1.f;
		const float half = 1.f; // height

		poss.push_back({ 0, half,0 });
		poss.push_back({ 0,-half,0 });
		if( genNors ) {
			poss.push_back({ 0, 1,0 });
			poss.push_back({ 0,-1,0 });
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
			float sideU = 2.f * i / (float)nrSlices;
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
			tris.push_back({ 0,          5+nrCols*i,     5+nrCols*(i+1) });
		}

		rst->initGL();
		return rst;
	}

	Mesh* genCapsule(int nrSlices, int nrStacks, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

		const float radius = 1.f;
		const float halfSylinder = 1.f; // height
		const int halfStacks = nrStacks / 2;
		nrStacks = halfStacks * 2;

		// phi : angle form xy-plane [-pi/2, pi/2]
		// theta : y-axis angle [0, 2pi]
		for( int stack=0; stack<=nrStacks; stack++ )
		{
			float phi = H_PI - F_PI * stack / (float)nrStacks;
			float y = sin(phi);
			float rcos = radius * cos(phi);
			for( int slice=0; slice<=nrSlices; slice++ )
			{
				float theta = D_PI * slice / (float)nrSlices;
				float x = rcos * cos(theta);
				float z = -rcos * sin(theta);
				vec3 pos = {x, y, z};
				vec3 nor = normalize(pos);
				vec2 uv = { 2.f*slice/(float)nrSlices, 1.f-stack/(float)nrStacks };
				pos.y += (stack<halfStacks)? halfSylinder : (-halfSylinder);
				uv.y += (stack<halfStacks)? 0.5f : (-0.5f);

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

		rst->initGL();

		return rst;
	}

	Mesh *genDonut(int nrSlices, int nrRingVerts, bool genNors, bool genUvs)
	{
		Mesh* rst = new Mesh();
		std::vector<vec3>& 	poss = rst->poss;
		std::vector<vec3>& 	nors = rst->nors;
		std::vector<vec2>& 	uvs  = rst->uvs;
		std::vector<uvec3>& tris = rst->tris;

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

		rst->initGL();

		return rst;
	}
}