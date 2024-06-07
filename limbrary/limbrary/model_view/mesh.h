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
		static constexpr int MAX_BONE_PER_VERT = 4;
		struct VertBoneInfo {
			int ids[MAX_BONE_PER_VERT] = { -1, };
			float weights[MAX_BONE_PER_VERT] = { 0.f,};
		};
	public:
		std::string name = "unnamed mesh";

		std::vector<glm::vec3> poss;
		std::vector<glm::vec3> nors;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> cols;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<VertBoneInfo> bone_infos;
		std::vector<glm::uvec3> tris;
		GLuint pos_buf = 0; 			// 0
		GLuint nor_buf = 0; 			// 1
		GLuint uv_buf  = 0;				// 2
		GLuint color_buf = 0;			// 3
		GLuint tangent_buf = 0; 		// 4
		GLuint bitangent_buf = 0;		// 5
		GLuint bone_infos_buf = 0;		// 6, 7
		GLuint element_buf = 0;
		GLuint vert_array = 0;

	public:
		Mesh(Mesh&& src)			 = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&& src)  = delete;
		
		Mesh();
		Mesh(const Mesh& src); // clone
		virtual ~Mesh();
		
		void initGL();
		void deinitGL();
		void print() const;
		void bindGL() const;
		void drawGL() const;
		void bindAndDrawGL() const;
	};
}

#endif