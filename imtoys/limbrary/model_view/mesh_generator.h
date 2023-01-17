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
		/* strip mesh from opengl
		void renderSphere()
		{
			static unsigned int sphereVAO = 0;
			static unsigned int indexCount = 0;

			if( glIsVertexArray(sphereVAO)==GL_FALSE ) {
				unsigned int vbo, ebo;

				glGenVertexArrays(1, &sphereVAO);
				glGenBuffers(1, &vbo);
				glGenBuffers(1, &ebo);

				std::vector<glm::vec3> positions;
				std::vector<glm::vec2> uv;
				std::vector<glm::vec3> normals;
				std::vector<unsigned int> indices;

				const unsigned int X_SEGMENTS = 64;
				const unsigned int Y_SEGMENTS = 64;
				const float PI = 3.14159265359;
				for( unsigned int y = 0; y <= Y_SEGMENTS; ++y ) {
					for( unsigned int x = 0; x <= X_SEGMENTS; ++x ) {
						float xSegment = (float)x / (float)X_SEGMENTS;
						float ySegment = (float)y / (float)Y_SEGMENTS;
						float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
						float yPos = std::cos(ySegment * PI);
						float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

						positions.push_back(glm::vec3(xPos, yPos, zPos));
						uv.push_back(glm::vec2(xSegment, ySegment));
						normals.push_back(glm::vec3(xPos, yPos, zPos));
					}
				}

				bool oddRow = false;
				for( unsigned int y = 0; y < Y_SEGMENTS; ++y ) {
					if( !oddRow ) // even rows: y == 0, y == 2; and so on
					{
						for( unsigned int x = 0; x <= X_SEGMENTS; ++x ) {
							indices.push_back(y       * (X_SEGMENTS + 1) + x);
							indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
						}
					} else {
						for( int x = X_SEGMENTS; x >= 0; --x ) {
							indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
							indices.push_back(y       * (X_SEGMENTS + 1) + x);
						}
					}
					oddRow = !oddRow;
				}
				indexCount = (int)indices.size();

				std::vector<float> data;
				for( unsigned int i = 0; i < positions.size(); ++i ) {
					data.push_back(positions[i].x);
					data.push_back(positions[i].y);
					data.push_back(positions[i].z);
					if( normals.size() > 0 ) {
						data.push_back(normals[i].x);
						data.push_back(normals[i].y);
						data.push_back(normals[i].z);
					}
					if( uv.size() > 0 ) {
						data.push_back(uv[i].x);
						data.push_back(uv[i].y);
					}
				}
				glBindVertexArray(sphereVAO);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
				float stride = (3 + 3 + 2) * sizeof(float);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
			}

			glEnable(GL_DEPTH_TEST);
			glBindVertexArray(sphereVAO);
			glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
		}*/
	};
}

#endif