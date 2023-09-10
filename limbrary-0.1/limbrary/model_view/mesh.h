//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#ifndef __mesh_h_
#define __mesh_h_

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "material.h"

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

			Vertex &operator=(const Vertex &copy);
		};
	}

	class Mesh
	{
	public:
		std::string name;
		std::vector<n_mesh::Vertex> vertices;
		std::vector<GLuint> indices;
		Material* material;
		GLenum draw_mode = GL_TRIANGLES;

		GLuint VAO, VBO, EBO;
		unsigned int ai_mat_idx;

	private:
		// disable copying
		Mesh(Mesh const &) = delete;
		Mesh &operator=(Mesh const &) = delete;
	public:
		Mesh();
		// shared vertex triangle mesh
		Mesh(const std::vector<n_mesh::Vertex> &_vertices, const std::vector<GLuint> &_indices, const std::vector<std::shared_ptr<Texture>> &_textures, const std::string_view _name = "");
		~Mesh();
		void clear();
		// upload VRAM
		void initGL();
		void deinitGL();
		void print() const;
		void replicateExtraData(const Mesh &target);
	};
}

#endif