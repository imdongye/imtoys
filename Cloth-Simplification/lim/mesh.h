//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#ifndef MESH_H
#define MESH_H

namespace lim
{
	namespace n_mesh
	{
		const int MAX_BONE_INFLUENCE = 4;
		// 4byte * (3+3+2+3+3+4+4) = 88
		// offsetof(Vertex, Normal) = 12
		struct Vertex
		{
			glm::vec3 p, n;
			glm::vec2 uv;
			glm::vec3 tangent, bitangent;
			int m_BoneIDs[MAX_BONE_INFLUENCE];
			float m_Weights[MAX_BONE_INFLUENCE];
			float bendingFactor; // 1 : 많이접힘, 0 : 

			Vertex& operator=(const Vertex& copy)
			{
				p=copy.p; n=copy.n;
				uv=copy.uv;
				tangent=copy.tangent; bitangent = copy.bitangent;
				memcpy(m_BoneIDs, copy.m_BoneIDs, sizeof(int) * MAX_BONE_INFLUENCE);
				memcpy(m_Weights, copy.m_Weights, sizeof(float) * MAX_BONE_INFLUENCE);
				return *this;
			}
		};
	}

	class Mesh
	{
	public:
		std::string name;
		std::vector<n_mesh::Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<GLuint> texIdxs;
		GLuint angles=3; // set size of indices
		glm::vec3 color; // Kd, diffuse color
		int hasTexture = 1;
	private:
		GLuint VAO, VBO, EBO;
		GLenum drawMode;
		friend class ModelLoader;
		friend class ModelExporter;
		unsigned int aiMatIdx;
	private:
		// disable copying
		Mesh(Mesh const&) = delete;
		Mesh& operator=(Mesh const&) = delete;
	public:
		Mesh(const std::string_view _name="")
			: VAO(0), name(_name)
		{
			// todo: apply every face diff draw mode
			switch( angles ) {
			case 3: drawMode = GL_TRIANGLES; break;
			case 2: drawMode = GL_LINE_STRIP; break;
			case 4: drawMode = GL_TRIANGLE_FAN; break;
			}
		}
		Mesh(const std::vector<n_mesh::Vertex>& _vertices
			 , const std::vector<GLuint>& _indices
			 , const std::vector<GLuint>& _texIdxs
			 , const std::string_view _name="")
			: Mesh(_name)
		{
			vertices = _vertices; // todo: fix deep copy
			indices = _indices;
			texIdxs = _texIdxs;
			hasTexture = (_texIdxs.size()>0)?1:0;
			setupMesh();
		}
		// copy without mesh data
		Mesh(const Mesh* mesh)
		{
			drawMode = mesh->drawMode;
			color = mesh->color;
			texIdxs = mesh->texIdxs;
			aiMatIdx = mesh->aiMatIdx;
		}
		~Mesh()
		{
			clear();
		}
		void clear()
		{
			vertices.clear();
			indices.clear();
			texIdxs.clear();
			if( VAO!=0 ) {
				glDeleteVertexArrays(1, &VAO);
				VAO=0;
			}
		}
		void draw(const GLuint pid=0, const std::vector<Texture>& textures_loaded=std::vector<Texture>())
		{
			/* shadowMap draw할때 pid=0 으로 해서 텍스쳐 uniform 안함 */
			if( pid != 0 ) {
				int slotCounter=0;
				GLuint diffuseNr  = 0;
				GLuint specularNr = 0;
				GLuint normalNr   = 0;
				GLuint ambientNr  = 0;
				for( GLuint i : texIdxs ) {
					std::string type = textures_loaded[i].type;
					// uniform samper2d nr is start with 0
					int backNum = 0;
					if( type=="map_Kd" )        backNum = diffuseNr++;
					else if( type=="map_Ks" )   backNum = specularNr++;
					else if( type=="map_Bump" ) backNum = normalNr++;
					else if( type=="map_Ka" )   backNum = ambientNr++;

					std::string varName = type + std::to_string(backNum);
					glActiveTexture(GL_TEXTURE0 + slotCounter); // slot
					glBindTexture(GL_TEXTURE_2D, textures_loaded[i].id);
					setUniform(pid, varName.c_str(), slotCounter++);// to sampler2d
				}
				glActiveTexture(GL_TEXTURE0);
				setUniform(pid, "texCount", (int)texIdxs.size());
				setUniform(pid, "Kd", color);
			}

			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glDrawElements(drawMode, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}

		// upload VRAM
		void setupMesh()
		{
			const size_t SIZE_OF_VERTEX = sizeof(n_mesh::Vertex);
			if( VAO!=0 )
				glDeleteVertexArrays(1, &VAO);
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * SIZE_OF_VERTEX, &vertices[0], GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			// VAO setting //
			// - position
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)0);
			// - normal
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, n));
			// - tex coord
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, uv));
			// - tangent
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, tangent));
			// - bi tangnet
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, bitangent));

			// - bone ids
			glEnableVertexAttribArray(5);
			glVertexAttribIPointer(5, n_mesh::MAX_BONE_INFLUENCE, GL_INT, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, m_BoneIDs));
			// - weights
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, n_mesh::MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void*)offsetof(n_mesh::Vertex, m_Weights));

			glBindVertexArray(0);

			// 이게 왜 가능한거지
			//glDeleteBuffers(1, &VBO);
			//glDeleteBuffers(1, &EBO);
		}
		void print() const
		{
			Logger::get().log("%s, verts %d, tris %d\n", name.c_str(), vertices.size(), indices.size()/3);
		}
	};
}

#endif