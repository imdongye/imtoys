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

#ifndef MESH_GEN_H
#define MESH_GEN_H

namespace lim
{
	class MeshGenerator
	{
	private: // buffers
		inline static std::vector<n_mesh::Vertex> vertices;
		inline static std::vector<GLuint> indices;
		inline static std::vector<std::shared_ptr<Texture>> textures;
		static void clearBuf()
		{
			static bool first = true;
			if( first ) {
				first = false;
				vertices.reserve(100);
				indices.reserve(100);
			}
			vertices.clear();
			indices.clear();
			textures.clear();
		}
	public:
		static Mesh* genQuad()
		{
			const float half = 1.0f;
			const glm::vec3 front ={0,0,1};

			clearBuf();

			vertices.push_back({{-half,  half, 0}, front, {0,1}});
			vertices.push_back({{ half,  half, 0}, front, {1,1}});
			vertices.push_back({{-half, -half, 0}, front, {0,0}});
			vertices.push_back({{ half, -half, 0}, front, {1,0}});

			indices.insert(indices.end(), {0,3,1});
			indices.insert(indices.end(), {0,2,3});

			return new Mesh(vertices, indices, textures);
		}
		static Mesh* genCube()
		{
			const float half = 1.0f;
			const glm::vec3 nors[6] ={
				{0,1,0},{0,0,1},{1,0,0},
				{-1,0,0},{0,0,-1},{0,-1,0}
			};
			const glm::vec3 tans[6] ={
				{1,0,0},{1,0,0},{0,0,-1},
				{0,0,1},{-1,0,0},{-1,0,0}
			};

			clearBuf();

			for( int i=0; i<6; i++ ) {
				glm::vec3 n = nors[i];
				glm::vec3 t = tans[i];
				glm::vec3 b = cross(n, t);

				vertices.push_back({  t*half +b*half +n*half,  n, {1,1}});
									 						  
				vertices.push_back({ -t*half +b*half +n*half,  n, {0,1}});
									 						  
				vertices.push_back({ -t*half -b*half +n*half,  n, {0,0}});
									 						  
				vertices.push_back({  t*half -b*half +n*half,  n, {1,0}});
			}
			for( unsigned int i=0; i<6; i++ ) {
				indices.insert(indices.end(), {0+i*4, 1+i*4, 2+i*4});
				indices.insert(indices.end(), {0+i*4, 2+i*4, 3+i*4});
			}

			return new Mesh(vertices, indices, textures);
		}

		// From: http://www.songho.ca/opengl/gl_sphere.html
		// texture coord가 다른 같은 위치의 vertex가 많음
		static Mesh* genSphere(int nrSlices=50, int nrStacks=25)
		{
			const float radius = 1.f;
			clearBuf();

			// phi : angle form xy-plane [-pi/2, pi/2]
			// theta : y-axis angle [0, 2pi]
			for( int stack=0; stack<=nrStacks; stack++ ) {
				float phi = H_PI - F_PI * stack/(float)nrStacks;
				float y = sinf(phi);
				float rcos = radius*cosf(phi);
				for( int slice=0; slice<=nrSlices; slice++ ) {
					float theta = D_PI * slice/(float)nrSlices;
					float x = rcos * cosf(theta);
					float z = -rcos * sinf(theta);
					glm::vec3 pos ={x, y, z};
					glm::vec3 norm = glm::normalize(pos);
					glm::vec2 uv ={2.f*slice/(float)nrSlices, 1.f-stack/(float)nrStacks};
					vertices.push_back({pos, norm, uv});
				}
			}

			const int nr_cols = nrSlices+1;

			for( int stack=0; stack<nrStacks; stack++ ) {
				int cur_row = nr_cols*stack;
				int next_row = nr_cols*(stack+1);
				for( int slice=0; slice<nrSlices; slice++ ) {
					int cur_col = slice;
					int next_col = slice+1;
					if( stack<nrStacks ) { // up tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + cur_col);
						indices.push_back(next_row + next_col);
					}
					if( stack>0 ) { // inv tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + next_col);
						indices.push_back(cur_row + next_col);
					}
				}
			}

			return new Mesh(vertices, indices, textures);
		}

		static Mesh* genIcoSphere(int subdivision=0)
		{
			const float uStep = 1.f/11.f;
			const float vStep = 1.f/3.f;
			const float radius = 1.f;
			clearBuf();
            
			for( int i=0; i<5; i++ ) {
				vertices.push_back({ {0, radius,0}, {0, 1,0}, {uStep*(1+2*i), 1} });
				vertices.push_back({ {0,-radius,0}, {0,-1,0}, {uStep*(3+2*i), 0} });
			}
            
            glm::vec3 pos, norm;  glm::vec2 uv;
            const float aStep = D_PI/5.f; // angle step
            const float halfH = radius*0.5f; // half height
            const float base = halfH*sqrtf(3); // bottom length 밑변
            
            float topA = 0; // top angle
            float botA = aStep*0.5f;
            
			for( int i=0; i<6; i++ ) {
                pos = { base*cosf(topA), halfH, base*sinf(topA) };
                norm = glm::normalize(pos);
                uv =  { uStep*(2*i), 1 };
                vertices.push_back({pos, norm, uv});
                topA += aStep;
                
                pos = { base*cosf(botA), halfH, base*sinf(botA) };
                norm = glm::normalize(pos);
                uv =  { uStep*(1+2*i), 0 };
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
                21, 20, 29,
                
                1, 13, 11,
                3, 15, 13,
                5, 17, 15,
                7, 19, 17,
                9, 21, 19,
            };

			return new Mesh(vertices, indices, textures);
		}

		static Mesh* genCylinder(int nrSlices=50)
		{
			const float radius = 1.f;
			const float half = 1.f; // height

			clearBuf();

			vertices.push_back({ {0, half, 0}, {0,1,0}, {.5f,.5f} });
			vertices.push_back({ {0,-half, 0}, {0,-1,0}, {.5f,.5f} });

			for( int slice=0; slice<=nrSlices; slice++ ) {
				float theta = D_PI * slice/(float)nrSlices;
				float x = radius * cosf(theta);
				float z = -radius * sinf(theta);
				glm::vec3 pos;
				glm::vec3 sideNor = glm::normalize(glm::vec3(x, 0, z));
				float sideU = 2.f*slice/(float)nrSlices;
				glm::vec2 circleUv = {x,-z};
				circleUv = .5f*(circleUv+glm::vec2(1));


				pos ={x, half,z};
				vertices.push_back({pos, {0,1,0},  circleUv});
				vertices.push_back({pos, sideNor, {sideU,1}});

				pos ={x,-half,z};
				vertices.push_back({pos, sideNor, {sideU,0}});
				vertices.push_back({pos, {0,-1,0}, circleUv});
			}
			const unsigned int nrCols = 4;
			for( unsigned int slice=0; slice<nrSlices; slice++ ) {
				indices.insert(indices.end(), {0, 2+nrCols*slice, 2+nrCols*(slice+1)});
				indices.insert(indices.end(), {3+nrCols*slice, 4+nrCols*slice, 4+nrCols*(slice+1)}); // up
				indices.insert(indices.end(), {3+nrCols*slice, 4+nrCols*(slice+1), 3+nrCols*(slice+1)}); // inv
				indices.insert(indices.end(), {1, 5+nrCols*slice, 5+nrCols*(slice+1)});
			}

			return new Mesh(vertices, indices, textures);
		}

		static Mesh* genCapsule(int nrSlices=50, int nrStacks=25)
		{
			const float radius = 1.f;
			const float halfSylinder = 1.f; // height
			const int halfStacks = nrStacks/2;
			nrStacks = halfStacks*2;

			clearBuf();
			
			// phi : angle form xy-plane [-pi/2, pi/2]
			// theta : y-axis angle [0, 2pi]
			for( int stack=0; stack<=nrStacks; stack++ ) {
				float phi = H_PI - F_PI * stack/(float)nrStacks;
				float y = sinf(phi);
				float rcos = radius*cosf(phi);
				for( int slice=0; slice<=nrSlices; slice++ ) {
					float theta = D_PI * slice/(float)nrSlices;
					float x = rcos * cosf(theta);
					float z = -rcos * sinf(theta);
					glm::vec3 pos ={x,y,z};
					glm::vec3 norm = glm::normalize(pos);
					glm::vec2 uv ={2.f*slice/(float)nrSlices, 1.f-stack/(float)nrStacks};
					if( stack<halfStacks ) {
						pos.y += halfSylinder;
						uv.y += 0.5f;
					}
					else {
						pos.y -= halfSylinder;
						uv.y -= 0.5f;
					}
					vertices.push_back({pos, norm, uv});
				}
			}

			const int nrCols = nrSlices+1;

			for( int stack=0; stack<nrStacks; stack++ ) {
				int cur_row = nrCols*stack;
				int next_row = nrCols*(stack+1);
				for( int slice=0; slice<nrSlices; slice++ ) {
					int cur_col = slice;
					int next_col = slice+1;
					if( stack<nrStacks ) { // up tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + cur_col);
						indices.push_back(next_row + next_col);
					}
					if( stack>0 ) { // inv tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + next_col);
						indices.push_back(cur_row + next_col);
					}
				}
			}

			return new Mesh(vertices, indices, textures);
		}
		static Mesh* genDonut(int nrSlices=50, int nrRingVerts=10)
		{
			const float ringRad = 1.f;
			const float donutRad = 1.5f;

			clearBuf();

			// calculus : shell method
			for( int slice=0; slice<=nrSlices; slice++ ) {
				float donutTheta = -D_PI * slice/(float)nrSlices;
				for( int rv=0; rv<=nrRingVerts; rv++ ) {
					float ringTheta = F_PI + D_PI * rv/(float)nrRingVerts;
					float y =  ringRad*sinf(ringTheta);
					float relativeX = donutRad + ringRad*cosf(ringTheta);
					float x = relativeX * cosf(donutTheta);
					float z = relativeX * sin(donutTheta);

					glm::vec3 pos ={x,y,z};
					glm::vec3 norm = glm::normalize(pos);
					glm::vec2 uv ={2.f*slice/(float)nrSlices, rv/(float)nrRingVerts};

					vertices.push_back({pos, norm, uv});
				}
			}
			int nrRealRingVerts = nrRingVerts+1;
			for( int slice=0; slice<nrSlices; slice++ ) {
				int curRing = nrRealRingVerts*slice;
				int nextRing = curRing+nrRealRingVerts;
				for( int rv=0; rv<nrRingVerts; rv++ ) {
					int curVert = rv;
					int nextVert = rv+1;
					// up tri
					indices.push_back(curRing  + curVert);
					indices.push_back(nextRing + curVert);
					indices.push_back(nextRing + nextVert);
					// inv tri
					indices.push_back(curRing  + curVert);
					indices.push_back(nextRing + nextVert);
					indices.push_back(curRing  + nextVert);
				}
			}

			return new Mesh(vertices, indices, textures);
		}
	};
}

#endif
