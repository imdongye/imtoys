/*

2022-07-20 / im dong ye
shared vertex triangle mesh

Note:
Material은 Mesh에 종속적이지 않음.

*/

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
	// clone able
	class Mesh
	{
	public:
		static constexpr int MAX_BONE_INFLUENCE = 4;

		std::string name = "unnamed mesh";

		std::vector<glm::vec3> poss;
		std::vector<glm::vec3> nors;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> cols;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<std::array<int, MAX_BONE_INFLUENCE>> bone_ids;
		std::vector<std::array<float, MAX_BONE_INFLUENCE>> bending_factors;
		std::vector<glm::uvec3> tris;
		GLuint pos_buf = 0;
		GLuint nor_buf = 0;
		GLuint uv_buf  = 0;
		GLuint color_buf = 0;
		GLuint tangent_buf = 0;
		GLuint bitangent_buf = 0;
		GLuint bone_id_buf = 0;
		GLuint bending_factor_buf = 0;
		GLuint element_buf = 0;
		GLuint vert_array = 0;

	public:
		Mesh(Mesh&& src)			 = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&& src)  = delete;
		
		Mesh();
		Mesh(const Mesh& src); // clone
		virtual ~Mesh();
		
		void drawGL() const;
		void initGL();
		void deinitGL();
		void print() const;
	};
}

#endif