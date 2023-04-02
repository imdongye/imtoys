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
			glm::vec2 uv; // texture coordinate
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
		std::vector<std::shared_ptr<Texture>> textures;
		GLuint angles=3; // set size of indices
		glm::vec3 color; // Kd, diffuse color
		int has_texture = 1;
		GLenum draw_mode;
	private:
		GLuint VAO, VBO, EBO;
		unsigned int ai_mat_idx;

		friend class ModelLoader;
		friend class ModelExporter;
		friend class MeshGenerator;
	private:
		// disable copying
		Mesh(Mesh const&) = delete;
		Mesh& operator=(Mesh const&) = delete;
	public:
		Mesh()
		{
		}
		// shared vertex triangle mesh
		Mesh(const std::vector<n_mesh::Vertex>& _vertices
			 , const std::vector<GLuint>& _indices
			 , const std::vector<std::shared_ptr<Texture>>& _textures
			 , const std::string_view _name="")
			: name(_name)
		{
			draw_mode = GL_TRIANGLES;
			vertices = _vertices; // todo: fix deep copy
			indices = _indices;
			textures = _textures;
			has_texture = (textures.size()>0)?1:0;
			setupMesh();
		}
		~Mesh()
		{
			clear();
		}
		void clear()
		{
			vertices.clear();
			indices.clear();
			textures.clear();
			if( VAO!=0 ) {
				glDeleteVertexArrays(1, &VAO);
				VAO=0;
			}
		}
		void draw(const GLuint pid=0)
		{
			/* shadowMap draw할때 pid=0 으로 해서 텍스쳐 uniform 안함 */
			if( pid != 0 ) {
				int slotCounter=0;
				GLuint diffuseNr  = 0;
				GLuint specularNr = 0;
				GLuint normalNr   = 0;
				GLuint ambientNr  = 0;
				for( std::shared_ptr<Texture> tex : textures ) {
					std::string& type = tex->tag;
					// uniform samper2d nr is start with 0
					int backNum = 0;
					if( type=="map_Kd" )        backNum = diffuseNr++;
					else if( type=="map_Ks" )   backNum = specularNr++;
					else if( type=="map_Bump" ) backNum = normalNr++;
					else if( type=="map_Ka" )   backNum = ambientNr++;

					std::string varName = type + std::to_string(backNum);
					glActiveTexture(GL_TEXTURE0 + slotCounter); // slot
					glBindTexture(GL_TEXTURE_2D, tex->tex_id);
					setUniform(pid, varName.c_str(), slotCounter++);// to sampler2d
				}
				setUniform(pid, "texCount", (int)textures.size());
				setUniform(pid, "Kd", color);
			}

			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glDrawElements(draw_mode, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
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

			// 이게 왜 가능한거지 IN WINDOWS
			//glDeleteBuffers(1, &VBO);
			//glDeleteBuffers(1, &EBO);
		}
		void print() const
		{
			Logger::get().log("%s, verts %d, tris %d\n", name.c_str(), vertices.size(), indices.size()/3);
		}
		void replicateExtraData(const Mesh& target)
		{
			draw_mode = target.draw_mode;
			color = target.color;
			textures = target.textures;
			ai_mat_idx = target.ai_mat_idx;
		}
	};
}

#endif