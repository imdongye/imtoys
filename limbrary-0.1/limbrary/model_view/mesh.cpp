//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#include <limbrary/model_view/mesh.h>
#include <limbrary/log.h>
#include <limbrary/texture.h>
#include <limbrary/program.h>

namespace lim
{
	namespace n_mesh
	{
		Vertex &Vertex::operator=(const Vertex &copy)
		{
			p = copy.p;
			n = copy.n;
			uv = copy.uv;
			tangent = copy.tangent;
			bitangent = copy.bitangent;
			memcpy(m_BoneIDs, copy.m_BoneIDs, sizeof(int) * MAX_BONE_INFLUENCE);
			memcpy(m_Weights, copy.m_Weights, sizeof(float) * MAX_BONE_INFLUENCE);
			return *this;
		}
	}

	Mesh::Mesh()
	{
	}
	// shared vertex triangle mesh
	Mesh::Mesh(const std::vector<n_mesh::Vertex> &_vertices, const std::vector<GLuint> &_indices, const std::vector<std::shared_ptr<Texture>> &_textures, const std::string_view _name)
		: name(_name)
	{
		draw_mode = GL_TRIANGLES;
		vertices = _vertices; // todo: fix deep copy
		indices = _indices;
		textures = _textures;
		has_texture = (textures.size() > 0) ? 1 : 0;
		setupMesh();
	}
	Mesh::~Mesh()
	{
		clear();
	}
	void Mesh::clear()
	{
		vertices.clear();
		indices.clear();
		textures.clear();
		if (VAO != 0)
		{
			glDeleteVertexArrays(1, &VAO);
			VAO = 0;
		}
	}
	// upload VRAM
	void Mesh::setupMesh()
	{
		const size_t SIZE_OF_VERTEX = sizeof(n_mesh::Vertex);
		if (VAO != 0)
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
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)0);
		// - normal
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, n));
		// - tex coord
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, uv));
		// - tangent
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, tangent));
		// - bi tangnet
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, bitangent));

		// - bone ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, n_mesh::MAX_BONE_INFLUENCE, GL_INT, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, m_BoneIDs));
		// - weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, n_mesh::MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, SIZE_OF_VERTEX, (void *)offsetof(n_mesh::Vertex, m_Weights));

		// 이게 왜 가능한거지 IN WINDOWS
		// glDeleteBuffers(1, &VBO);
		// glDeleteBuffers(1, &EBO);
	}
	void Mesh::print() const
	{
		log::info("%s, verts %d, tris %d\n", name.c_str(), vertices.size(), indices.size() / 3);
	}
	void Mesh::replicateExtraData(const Mesh &target)
	{
		draw_mode = target.draw_mode;
		color = target.color;
		textures = target.textures;
		ai_mat_idx = target.ai_mat_idx;
	}
}