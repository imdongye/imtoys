/*

2022-07-20 / im dong ye
shared vertex triangle mesh

Note:
Material은 Mesh에 종속적이지 않음.
poss, nors buffer element size = vec4 for ssbo


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
			int idxs[MAX_BONE_PER_VERT] = { -1, };
			float weights[MAX_BONE_PER_VERT] = { 0.f,};
		};
	public:
		std::string name = "unnamed mesh";

		std::vector<glm::vec3> poss;
		std::vector<glm::vec3> nors;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<glm::vec3> cols;
		std::vector<VertBoneInfo> bone_infos;
		std::vector<glm::uvec3> tris;
		
		GLuint buf_pos = 0; 			// 0
		GLuint buf_nor = 0; 			// 1
		GLuint buf_uv  = 0;				// 2
		GLuint buf_color = 0;			// 3
		GLuint buf_tangent = 0; 		// 4
		GLuint buf_bitangent = 0;		// 5
		GLuint buf_bone_info = 0;		// 6, 7
		GLuint buf_tris = 0;
		GLuint vao = 0;

	public:
		Mesh(Mesh&& src)			 = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh& operator=(Mesh&& src)  = delete;
		
		Mesh();
		Mesh(const Mesh& src); // clone
		virtual ~Mesh();
		
		void initGL(bool withClearMem = false);
		void restorePosBuf();
		void deinitGL();
		void clearMem();
		void print() const;
		void bindGL() const;
		void drawGL() const;
		void bindAndDrawGL() const;
	};
}

#endif