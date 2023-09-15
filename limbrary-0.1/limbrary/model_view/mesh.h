//
//  2022-07-20 / im dong ye
//	shared vertex triangle mesh
//
//	todo :
//	1. bumpmap normalmap확인

#ifndef __mesh_h_
#define __mesh_h_

#include <string>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "material.h"

namespace lim
{
	class Mesh
	{
	public:
		std::string name = "unnamed mesh";

		std::vector<glm::vec3> poss;
		std::vector<glm::vec3> nors;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec2> cols;
		std::vector<glm::vec2> tangents;
		std::vector<glm::vec2> bitangents;
		GLuint pos_buf = 0;
		GLuint nor_buf = 0;
		GLuint uv_buf  = 0;
		GLuint color_buf = 0;
		GLuint tangent_buf = 0;
		GLuint bitangent_buf = 0;

		static constexpr int MAX_BONE_INFLUENCE = 4;
		std::vector<std::array<int, MAX_BONE_INFLUENCE>> bone_ids;
		std::vector<std::array<float, MAX_BONE_INFLUENCE>> bending_factors;

		GLuint bone_id_buf = 0;
		GLuint bending_factor_buf = 0;

		std::vector<glm::uvec3> tris;
		GLuint element_buf = 0;

		GLuint vert_array = 0;

		Material* material = nullptr;

	private:
		// disable copying
		Mesh(Mesh const &) = delete;
		Mesh &operator=(Mesh const &) = delete;

	public:
		Mesh();
		~Mesh();
		void draw() const;
		Mesh* clone();
		void initGL();
		void deinitGL();
		void print() const;
	};
}

#endif