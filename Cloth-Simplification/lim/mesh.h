//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
#ifndef MESH_H
#define MESH_H

#include "texture.h"

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
		std::vector<Texture> textures;
		GLuint angles; // set size of indices
	private:
		GLuint VAO, VBO, EBO;
		GLenum drawMode;
	private:
		// disable copying
		Mesh(Mesh const&) = delete;
		Mesh& operator=(Mesh const&) = delete;
	public:
		Mesh(const std::string& _name="", const GLuint& _angle=3)
			: VAO(0), name(_name), angles(_angle)
		{
			// todo: apply every face diff draw mode
			switch( angles )
			{
			case 3: drawMode = GL_TRIANGLES; break;
			case 2: drawMode = GL_LINE_STRIP; break;
			case 4: drawMode = GL_TRIANGLE_FAN; break;
			}
		}
		Mesh(const std::vector<n_mesh::Vertex>& _vertices
			 , const std::vector<GLuint>& _indices
			 , const std::vector<Texture>& _textures
			 , const std::string& _name="", const GLuint& _angle=3)
			: Mesh(_name, _angle)
		{
			vertices = _vertices; // todo: fix deep copy
			indices = _indices;
			textures = _textures;
			setupMesh();
		}
		~Mesh()
		{
			clear();
		}
		void clear()
		{
			// vector은 heap 에서 언제 사라지지?
			vertices.clear();
			indices.clear();
			textures.clear();
			if( VAO!=0 )
			{
				glDeleteVertexArrays(1, &VAO);
				VAO=0;
			}
		}
		void draw(const Program& program)
		{
			// texture unifrom var name texture_specularN ...
			GLuint diffuseNr  = 0;
			GLuint specularNr = 0;
			GLuint normalNr   = 0;
			GLuint ambientNr  = 0;
			GLuint loc = 0;

			for( GLuint i=0; i<textures.size(); i++ )
			{
				std::string type = textures[i].type;
				// uniform samper2d nr is start with 1
				// omit 0th
				int backNum = 0;
				if( type=="map_Kd" )        backNum = diffuseNr++;
				else if( type=="map_Ks" )   backNum = specularNr++;
				else if( type=="map_Bump" ) backNum = normalNr++;
				else if( type=="map_Ka" )   backNum = ambientNr++;

				std::string varName = type + std::to_string(backNum);
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
				loc = glGetUniformLocation(program.ID, varName.c_str());
				glUniform1i(loc, i); // to sampler2d
			}

			glBindVertexArray(VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glDrawElements(drawMode, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);

			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glActiveTexture(GL_TEXTURE0);
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
			fprintf(stdout, "%-18s, angles %d, verts %-7lu, tris %-7lu\n"
					, name.c_str(), angles, vertices.size(), indices.size()/3);
		}
	};
}

#endif