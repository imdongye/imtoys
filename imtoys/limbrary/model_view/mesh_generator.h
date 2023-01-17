//
//	for generate mesh of general shape
//  2023-01-17 / im dong ye
//  
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
			const float half = 0.5f;
			const glm::vec3 front ={0,0,1};

			clearBuf();

			vertices.push_back({{-half,  half, 0}, front, {0,0}});
			vertices.push_back({{ half,  half, 0}, front, {1,0}});
			vertices.push_back({{-half, -half, 0}, front, {0,1}});
			vertices.push_back({{ half, -half, 0}, front, {1,1}});

			indices.insert(indices.end(), {0,3,1});
			indices.insert(indices.end(), {0,2,3});

			return new Mesh(vertices, indices, textures);
		}
		static Mesh* genCube()
		{
			const float half = 0.5f;
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

				vertices.push_back({  t*half +b*half +n*half,  n, {1,0}});
									 						  
				vertices.push_back({ -t*half +b*half +n*half,  n, {0,0}});
									 						  
				vertices.push_back({ -t*half -b*half +n*half,  n, {0,1}});
									 						  
				vertices.push_back({  t*half -b*half +n*half,  n, {1,1}});
			}
			for( unsigned int i=0; i<6; i++ ) {
				indices.insert(indices.end(), {0+i*4, 1+i*4, 2+i*4});
				indices.insert(indices.end(), {0+i*4, 2+i*4, 3+i*4});
			}

			return new Mesh(vertices, indices, textures);
		}

		// From: http://www.songho.ca/opengl/gl_sphere.html
		// texture coord가 다른 같은 위치의 vertex가 많음
		static Mesh* genSphere(int nr_slices=50, int nr_stacks=25)
		{
			const float radius = 1.f;
			clearBuf();

			// phi : angle form xy-plane [-pi/2, pi/2]
			// theta : y-axis angle [0, 2pi]
			for( int stack=0; stack<=nr_stacks; stack++ ) {
				float phi = H_PI - PI * stack/(float)nr_stacks;
				float y = sinf(phi);
				float rcos = radius*cosf(phi);
				for( int slice=0; slice<=nr_slices; slice++ ) {
					float theta = D_PI * slice/(float)nr_slices;
					float x = rcos * cosf(theta);
					float z = -rcos * sinf(theta);
					glm::vec3 pos ={x, y, z};
					glm::vec3 norm = glm::normalize(pos);
					glm::vec2 uv ={slice/(float)nr_slices, stack/(float)nr_stacks};
					vertices.push_back({pos, norm, uv});
				}
			}

			const int nr_cols = nr_slices+1;

			for( int yy=0; yy<nr_stacks; yy++ ) {
				int cur_row = nr_cols*yy;
				int next_row = nr_cols*(yy+1);
				for( int xx=0; xx<nr_slices; xx++ ) {
					int cur_col = xx;
					int next_col = xx+1;
					if( yy<nr_stacks ) { // up tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + cur_col);
						indices.push_back(next_row + next_col);
					}
					if( yy>0 ) { // inv tri
						indices.push_back(cur_row  + cur_col);
						indices.push_back(next_row + next_col);
						indices.push_back(cur_row + next_col);
					}
				}
			}

			return new Mesh(vertices, indices, textures);
		}

		static Mesh* genCylinder(int nr_slices=50)
		{
			const float radius = 1.f;
			const float half = 1.f;

			clearBuf();

			vertices.push_back({ {0, half, 0}, {0,1,0}, {0,0} });
			vertices.push_back({ {0,-half, 0}, {0,-1,0}, {0,0} });

			for( int slice=0; slice<=nr_slices; slice++ ) {
				float theta = D_PI * slice/(float)nr_slices;
				float x = radius * cosf(theta);
				float z = -radius * sinf(theta);
				glm::vec3 pos, sideNor;
				sideNor = glm::normalize(glm::vec3(x, 0, z));

				pos ={x, half,z};
				vertices.push_back({pos, {0,1,0}, {x,z} });
				vertices.push_back({pos, sideNor, {slice/(float)nr_slices,0} });

				pos ={x,-half,z};
				vertices.push_back({pos, sideNor, {slice/(float)nr_slices,0}});
				vertices.push_back({pos, {0,-1,0}, {x,z}});
			}
			const unsigned int nr_cols = 4;
			for( unsigned int slice=0; slice<nr_slices; slice++ ) {
				indices.insert(indices.end(), {0, 2+nr_cols*slice, 2+nr_cols*(slice+1)});
				indices.insert(indices.end(), {3+nr_cols*slice, 4+nr_cols*slice, 4+nr_cols*(slice+1)}); // up
				indices.insert(indices.end(), {3+nr_cols*slice, 4+nr_cols*(slice+1), 3+nr_cols*(slice+1)}); // inv
				indices.insert(indices.end(), {1, 5+nr_cols*slice, 5+nr_cols*(slice+1)});
			}

			return new Mesh(vertices, indices, textures);
		}
	};
}

#endif