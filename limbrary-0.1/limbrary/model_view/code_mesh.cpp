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

namespace
{
	using namespace lim;
	static std::vector<n_mesh::Vertex> vertices;
	std::vector<GLuint> indices;
	std::vector<std::shared_ptr<Texture>> textures;

	void clearBuf()
	{
		static bool first = true;
		if (first)
		{
			first = false;
			vertices.reserve(100);
			indices.reserve(100);
		}
		vertices.clear();
		indices.clear();
		textures.clear();
	}
	n_mesh::Vertex genHalfVertex(const n_mesh::Vertex &v1, const n_mesh::Vertex &v2, float radius)
	{
		glm::vec3 pos = (v1.p + v2.p) * 0.5f;
		glm::vec3 norm = glm::normalize(pos);
		pos = norm * radius;
		glm::vec2 uv = (v1.uv + v2.uv) * 0.5f;
		return {pos, norm, uv};
	}
}

namespace lim::code_mesh
{
	Mesh *genQuad()
	{
		const float half = 1.0f;
		const glm::vec3 front = {0, 0, 1};

		clearBuf();

		vertices.push_back({{-half, half, 0}, front, {0, 1}});
		vertices.push_back({{half, half, 0}, front, {1, 1}});
		vertices.push_back({{-half, -half, 0}, front, {0, 0}});
		vertices.push_back({{half, -half, 0}, front, {1, 0}});

		indices.insert(indices.end(), {0, 3, 1});
		indices.insert(indices.end(), {0, 2, 3});

		return new Mesh(vertices, indices, textures);
	}

	Mesh *genPlane(int nrSlice)
	{
		const float length = 2.0f;
		const float start = -length / 2.f;
		const float step = length / nrSlice;
		const glm::vec3 up = {0, 1, 0};

		clearBuf();

		const float div = nrSlice + 1;
		for(int i = 0; i <= nrSlice; i++) for(int j = 0; j <= nrSlice; j++)
		{
			vertices.push_back({{start + step * j, 0, start + step * i}, up, {(j) / div, (div - i) / div}});
		}

		const int nrCols = nrSlice + 1;
		for (int i = 0; i < nrSlice; i++) for (int j = 0; j < nrSlice; j++)
		{
			// 0-1
			// |\|
			// 2-3
			const int ori = i * nrCols + j;
			indices.push_back(ori + 0);
			indices.push_back(ori + 0 + nrCols);
			indices.push_back(ori + 1 + nrCols);

			indices.push_back(ori + 0);
			indices.push_back(ori + 1 + nrCols);
			indices.push_back(ori + 1);
		}
		return new Mesh(vertices, indices, textures);
	}

	Mesh *genCube()
	{
		const float half = 1.0f;
		const glm::vec3 nors[6] = {
			{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
		const glm::vec3 tans[6] = {
			{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

		clearBuf();

		for (int i = 0; i < 6; i++)
		{
			glm::vec3 n = nors[i];
			glm::vec3 t = tans[i];
			glm::vec3 b = cross(n, t);

			vertices.push_back({t * half + b * half + n * half, n, {1, 1}});

			vertices.push_back({-t * half + b * half + n * half, n, {0, 1}});

			vertices.push_back({-t * half - b * half + n * half, n, {0, 0}});

			vertices.push_back({t * half - b * half + n * half, n, {1, 0}});
		}
		for (unsigned int i = 0; i < 6; i++)
		{
			indices.insert(indices.end(), {0 + i * 4, 1 + i * 4, 2 + i * 4});
			indices.insert(indices.end(), {0 + i * 4, 2 + i * 4, 3 + i * 4});
		}

		return new Mesh(vertices, indices, textures);
	}

	// From: http://www.songho.ca/opengl/gl_sphere.html
	// texture coord가 다른 같은 위치의 vertex가 많음
	Mesh *genSphere(const int nrSlices, const int nrStacks)
	{
		const float radius = 1.f;
		clearBuf();

		// phi : angle form xy-plane [-pi/2, pi/2]
		// theta : y-axis angle [0, 2pi]
		for (int stack = 0; stack <= nrStacks; stack++)
		{
			float phi = H_PI - F_PI * stack / (float)nrStacks;
			float y = sinf(phi);
			float rcos = radius * cosf(phi);
			for (int slice = 0; slice <= nrSlices; slice++)
			{
				float theta = D_PI * slice / (float)nrSlices;
				float x = rcos * cosf(theta);
				float z = -rcos * sinf(theta);
				glm::vec3 pos = {x, y, z};
				glm::vec3 norm = glm::normalize(pos);
				glm::vec2 uv = {2.f * slice / (float)nrSlices, 1.f - stack / (float)nrStacks};
				vertices.push_back({pos, norm, uv});
			}
		}

		const int nr_cols = nrSlices + 1;

		for (int stack = 0; stack < nrStacks; stack++)
		{
			int cur_row = nr_cols * stack;
			int next_row = nr_cols * (stack + 1);
			for (int slice = 0; slice < nrSlices; slice++)
			{
				int cur_col = slice;
				int next_col = slice + 1;
				if (stack < nrStacks)
				{ // up tri
					indices.push_back(cur_row + cur_col);
					indices.push_back(next_row + cur_col);
					indices.push_back(next_row + next_col);
				}
				if (stack > 0)
				{ // inv tri
					indices.push_back(cur_row + cur_col);
					indices.push_back(next_row + next_col);
					indices.push_back(cur_row + next_col);
				}
			}
		}

		return new Mesh(vertices, indices, textures);
	}

	// icosahedron, 20면체
	Mesh *genIcoSphere(const int subdivision)
	{
		const float uStep = 1.f / 11.f;
		const float vStep = 1.f / 3.f;
		const float radius = 1.f;
		clearBuf();

		vertices.reserve(22);

		for (int i = 0; i < 5; i++)
		{
			vertices.push_back({{0, radius, 0}, {0, 1, 0}, {uStep * (1 + 2 * i), 1}});
			vertices.push_back({{0, -radius, 0}, {0, -1, 0}, {uStep * (3 + 2 * i), 0}});
		}

		glm::vec3 pos, norm;
		glm::vec2 uv;
		const float aStep = D_PI / 5.f;		 // angle step
		const float halfH = radius * 0.5f;	 // half height
		const float base = halfH * sqrtf(3); // bottom length 밑변

		float topA = 0; // top angle
		float botA = aStep * 0.5f;

		for (int i = 0; i < 6; i++)
		{
			pos = {base * cosf(topA), halfH, -base * sinf(topA)};
			norm = glm::normalize(pos);
			uv = {uStep * (2 * i), 2 * vStep};
			vertices.push_back({pos, norm, uv});
			topA += aStep;

			pos = {base * cosf(botA), -halfH, -base * sinf(botA)};
			norm = glm::normalize(pos);
			uv = {uStep * (1 + 2 * i), vStep};
			vertices.push_back({pos, norm, uv});
			botA += aStep;
		}

		indices = {
			0, 10, 12,
			2, 12, 14,
			4, 14, 16,
			6, 16, 18,
			8, 18, 20,
			
			10, 11, 12,
			12, 13, 14,
			14, 15, 16,
			16, 17, 18,
			18, 19, 20,
			
			13, 12, 11,
			15, 14, 13,
			17, 16, 15,
			19, 18, 17,
			21, 20, 19,
			
			1, 13, 11,
			3, 15, 13,
			5, 17, 15,
			7, 19, 17,
			9, 21, 19,
		};

		for (int i = 0; i < subdivision; i++)
		{
			const std::vector<GLuint> tmpIndices = indices;
			indices.clear();
			for (int j = 0; j < tmpIndices.size(); j += 3)
			{
				const auto &v1 = vertices[tmpIndices[j]];
				const auto &v2 = vertices[tmpIndices[j + 1]];
				const auto &v3 = vertices[tmpIndices[j + 2]];
				const GLuint newIdx = vertices.size();

				vertices.push_back(genHalfVertex(v1, v2, radius));
				vertices.push_back(genHalfVertex(v2, v3, radius));
				vertices.push_back(genHalfVertex(v3, v1, radius));

				indices.push_back(tmpIndices[j + 0]);
				indices.push_back(newIdx + 0);
				indices.push_back(newIdx + 2);

				indices.push_back(tmpIndices[j + 1]);
				indices.push_back(newIdx + 1);
				indices.push_back(newIdx + 0);

				indices.push_back(tmpIndices[j + 2]);
				indices.push_back(newIdx + 2);
				indices.push_back(newIdx + 1);

				indices.push_back(newIdx + 0);
				indices.push_back(newIdx + 1);
				indices.push_back(newIdx + 2);
			}
		}

		return new Mesh(vertices, indices, textures);
	}

	Mesh *genCubeSphere(const int nrSlices)
	{
		const float radius = 1.f;

		const glm::vec3 nors[6] = {
			{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
		const glm::vec3 tans[6] = {
			{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

		clearBuf();

		for (int side = 0; side < 6; side++)
		{
			glm::vec3 n = nors[side];
			glm::vec3 t = tans[side];
			glm::vec3 b = cross(n, t);

			GLuint offset = vertices.size();

			//float sliceLength = 2.f / nrSlices;

			for (int y = 0; y <= nrSlices; y++)
			{
				for (int x = 0; x <= nrSlices; x++)
				{
					float dx = (float)x / nrSlices * 2.f - 1.f;
					float dy = (float)y / nrSlices * 2.f - 1.f;
					glm::vec3 pos = n + t * dx + b * dy;
					glm::vec3 nor = glm::normalize(pos);
					pos = nor * radius;
					glm::vec2 uv = {dx * 0.5f + 0.5f, dy * 0.5f + 0.5f};
					vertices.push_back({pos, nor, uv});
				}
			}
			const int nrCols = nrSlices + 1;
			for (int y = 0; y < nrSlices; y++)
			{
				const GLuint curRow = offset + y * nrCols;
				const GLuint nextRow = offset + (y + 1) * nrCols;
				for (int x = 0; x < nrSlices; x++)
				{

					indices.push_back(nextRow + x);
					indices.push_back(curRow + x);
					indices.push_back(curRow + x + 1);

					indices.push_back(nextRow + x);
					indices.push_back(curRow + x + 1);
					indices.push_back(nextRow + x + 1);
				}
			}
		}
		return new Mesh(vertices, indices, textures);
	}

	// smooth
	Mesh *genCubeSphere2(const int nrSlices)
	{
		const float radius = 1.f;

		const glm::vec3 nors[6] = {
			{0, 1, 0}, {0, 0, 1}, {1, 0, 0}, {-1, 0, 0}, {0, 0, -1}, {0, -1, 0}};
		const glm::vec3 tans[6] = {
			{1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0}};

		/* genUnitCubeSpherePositiveXFace */
		std::vector<glm::vec3> facePoints;

		for (int y = 0; y <= nrSlices; y++)
		{ // z-axis angle
			float phi = H_PI * (float)y / nrSlices - Q_PI;
			glm::vec3 n1 = {-sinf(phi), cos(phi), 0};
			for (int x = 0; x <= nrSlices; x++)
			{ // y-axis angle
				float theta = H_PI * (float)x / nrSlices - Q_PI;
				glm::vec3 n2 = {sinf(theta), 0, cos(theta)};
				facePoints.push_back(glm::normalize(glm::cross(n1, n2)));
			}
		}

		clearBuf();

		for (int side = 0; side < 6; side++)
		{
			const int nrCols = nrSlices + 1;
			const int offset = vertices.size();
			const glm::mat3 rotMat = glm::mat3(nors[side],
											   tans[side],
											   cross(nors[side], tans[side]));

			for (int y = 0; y <= nrSlices; y++) for (int x = 0; x <= nrSlices; x++)
			{
				glm::vec3 pos = rotMat * facePoints[nrCols * y + x];
				glm::vec2 uv = {x / (float)nrSlices, y / (float)nrSlices};
				vertices.push_back({radius * pos, pos, uv});
			}

			for (int y = 0; y < nrSlices; y++)
			{
				const GLuint curRow = offset + y * nrCols;
				const GLuint upRow = offset + (y + 1) * nrCols;
				for (int x = 0; x < nrSlices; x++)
				{

					indices.push_back(upRow + x);
					indices.push_back(curRow + x);
					indices.push_back(curRow + x + 1);

					indices.push_back(upRow + x);
					indices.push_back(curRow + x + 1);
					indices.push_back(upRow + x + 1);
				}
			}
		}

		return new Mesh(vertices, indices, textures);
	}

	Mesh *genCylinder(const int nrSlices)
	{
		const GLuint slices = nrSlices;
		const float radius = 1.f;
		const float half = 1.f; // height

		clearBuf();

		vertices.push_back({{0, half, 0}, {0, 1, 0}, {.5f, .5f}});
		vertices.push_back({{0, -half, 0}, {0, -1, 0}, {.5f, .5f}});

		for (GLuint slice = 0; slice <= slices; slice++)
		{
			float theta = D_PI * slice / (float)nrSlices;
			float x = radius * cosf(theta);
			float z = -radius * sinf(theta);
			glm::vec3 pos;
			glm::vec3 sideNor = glm::normalize(glm::vec3(x, 0, z));
			float sideU = 2.f * slice / (float)nrSlices;
			glm::vec2 circleUv = {x, -z};
			circleUv = .5f * (circleUv + glm::vec2(1));

			pos = {x, half, z};
			vertices.push_back({pos, {0, 1, 0}, circleUv});
			vertices.push_back({pos, sideNor, {sideU, 1}});

			pos = {x, -half, z};
			vertices.push_back({pos, sideNor, {sideU, 0}});
			vertices.push_back({pos, {0, -1, 0}, circleUv});
		}
		const GLuint nrCols = 4;
		for (GLuint slice = 0; slice < slices; slice++)
		{
			indices.insert(indices.end(), {0, 2 + nrCols * slice, 2 + nrCols * (slice + 1)});
			indices.insert(indices.end(), {3 + nrCols * slice, 4 + nrCols * slice, 4 + nrCols * (slice + 1)});		 // up
			indices.insert(indices.end(), {3 + nrCols * slice, 4 + nrCols * (slice + 1), 3 + nrCols * (slice + 1)}); // inv
			indices.insert(indices.end(), {1, 5 + nrCols * slice, 5 + nrCols * (slice + 1)});
		}

		return new Mesh(vertices, indices, textures);
	}

	Mesh *genCapsule(int nrSlices, int nrStacks)
	{
		const float radius = 1.f;
		const float halfSylinder = 1.f; // height
		const int halfStacks = nrStacks / 2;
		nrStacks = halfStacks * 2;

		clearBuf();

		// phi : angle form xy-plane [-pi/2, pi/2]
		// theta : y-axis angle [0, 2pi]
		for (int stack = 0; stack <= nrStacks; stack++)
		{
			float phi = H_PI - F_PI * stack / (float)nrStacks;
			float y = sinf(phi);
			float rcos = radius * cosf(phi);
			for (int slice = 0; slice <= nrSlices; slice++)
			{
				float theta = D_PI * slice / (float)nrSlices;
				float x = rcos * cosf(theta);
				float z = -rcos * sinf(theta);
				glm::vec3 pos = {x, y, z};
				glm::vec3 norm = glm::normalize(pos);
				glm::vec2 uv = {2.f * slice / (float)nrSlices, 1.f - stack / (float)nrStacks};
				if (stack < halfStacks)
				{
					pos.y += halfSylinder;
					uv.y += 0.5f;
				}
				else
				{
					pos.y -= halfSylinder;
					uv.y -= 0.5f;
				}
				vertices.push_back({pos, norm, uv});
			}
		}

		const int nrCols = nrSlices + 1;

		for (int stack = 0; stack < nrStacks; stack++)
		{
			int cur_row = nrCols * stack;
			int next_row = nrCols * (stack + 1);
			for (int slice = 0; slice < nrSlices; slice++)
			{
				int cur_col = slice;
				int next_col = slice + 1;
				if (stack < nrStacks)
				{ // up tri
					indices.push_back(cur_row + cur_col);
					indices.push_back(next_row + cur_col);
					indices.push_back(next_row + next_col);
				}
				if (stack > 0)
				{ // inv tri
					indices.push_back(cur_row + cur_col);
					indices.push_back(next_row + next_col);
					indices.push_back(cur_row + next_col);
				}
			}
		}

		return new Mesh(vertices, indices, textures);
	}

	Mesh *genDonut(int nrSlices, int nrRingVerts)
	{
		const float ringRad = 1.f;
		const float donutRad = 1.5f;

		clearBuf();

		// calculus : shell method
		for (int slice = 0; slice <= nrSlices; slice++)
		{
			float donutTheta = -D_PI * slice / (float)nrSlices;
			for (int rv = 0; rv <= nrRingVerts; rv++)
			{
				float ringTheta = F_PI + D_PI * rv / (float)nrRingVerts;
				float y = ringRad * sinf(ringTheta);
				float relativeX = donutRad + ringRad * cosf(ringTheta);
				float x = relativeX * cosf(donutTheta);
				float z = relativeX * sin(donutTheta);

				glm::vec3 pos = {x, y, z};
				glm::vec3 norm = glm::normalize(pos);
				glm::vec2 uv = {2.f * slice / (float)nrSlices, rv / (float)nrRingVerts};

				vertices.push_back({pos, norm, uv});
			}
		}
		int nrRealRingVerts = nrRingVerts + 1;
		for (int slice = 0; slice < nrSlices; slice++)
		{
			int curRing = nrRealRingVerts * slice;
			int nextRing = curRing + nrRealRingVerts;
			for (int rv = 0; rv < nrRingVerts; rv++)
			{
				int curVert = rv;
				int nextVert = rv + 1;
				// up tri
				indices.push_back(curRing + curVert);
				indices.push_back(nextRing + curVert);
				indices.push_back(nextRing + nextVert);
				// inv tri
				indices.push_back(curRing + curVert);
				indices.push_back(nextRing + nextVert);
				indices.push_back(curRing + nextVert);
			}
		}

		return new Mesh(vertices, indices, textures);
	}
}