/*

	2022-07-20 / im dong ye
	shared vertex triangle mesh

Note:
	Material은 Mesh에 종속적이지 않음.
	vertex is SOA not AOS
	nr_verts, nr_tris update in initGL()

*/

#ifndef __mesh_h_
#define __mesh_h_

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "material.h"

namespace lim
{
	// clone able
	struct Mesh
	{
		static constexpr int MAX_BONE_PER_VERT = 4;
		struct VertBoneInfo {
			int idxs[MAX_BONE_PER_VERT] = { -1, };
			float weights[MAX_BONE_PER_VERT] = { 0.f,};
		};

		std::string name = "unnamed mesh";

		std::vector<glm::vec3> poss;
		std::vector<glm::vec3> nors;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;
		std::vector<glm::vec3> cols;
		std::vector<VertBoneInfo> bone_infos;
		std::vector<glm::ivec3> tris;

		// update in initGL()
		int nr_verts, nr_tris; 
		GLuint buf_poss = 0; 			// 0
		GLuint buf_nors = 0; 			// 1
		GLuint buf_uvs  = 0;			// 2
		GLuint buf_colors = 0;			// 3
		GLuint buf_tangents = 0; 		// 4
		GLuint buf_bitangents = 0;		// 5
		GLuint buf_bone_infos = 0;		// 6, 7
		GLuint buf_tris = 0;
		GLuint vao = 0;
		
		Mesh();
		Mesh(const Mesh& src); // clone
		Mesh& operator=(const Mesh& src);
		Mesh(Mesh&& src) noexcept; // used in SoftBody
		Mesh& operator=(Mesh&& src) noexcept;
		virtual ~Mesh();
		
		void initGL(bool withClearMem = false);
		void clearMem();
		void deinitGL();

		void bindGL() const;
		void drawGL() const;
		void bindAndDrawGL() const;

		void print() const;
		void updateNorsFromTris();
		// becareful memory fragmentation
		void subdivide(int level = 1);
	};
}

#endif