//
//	2022-08-07 / im dong ye
//
//	TODO list:
//  1. fix with half edge data structure
//
//

#ifndef __simplify_h_
#define __simplify_h_

#include "fqms.h"
#include <limbrary/model_view/model.h>
#include <limbrary/logger.h>

namespace fqms
{
	void updateGlobal(lim::Mesh *mesh)
	{
		vertices.clear();
		triangles.clear();

		for( lim::n_mesh::Vertex &v : mesh->vertices ) {
			Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			vertices.push_back(temp);
		}
		// make triangles & edge
		triangles.resize(mesh->indices.size() / 3);
		int triCount = 0;
		int vertCount = 0;
		for( GLuint idx : mesh->indices ) {
			lim::n_mesh::Vertex &overt = mesh->vertices[idx]; // origin
			Triangle &ttri = triangles[triCount];
			ttri.v[vertCount] = idx;
			ttri.uvs[vertCount] = vec3f(overt.uv.x, overt.uv.y, 1);
			ttri.n = vec3f(overt.n.x, overt.n.y, overt.n.z);
			vertCount++;
			if( vertCount == 3 ) {
				vertCount = 0;
				triCount++;
			}
		}
		if( triCount * 3 != mesh->indices.size() || mesh->indices.size() % 3 != 0 ) {
			lim::Log::get().log(stderr, "simplify failed : mesh is not triangle mesh\n");
		}
	}
	void loadFromGlobal(lim::Mesh *mesh)
	{
		mesh->vertices.clear();
		mesh->indices.clear();

		for( Vertex &v : vertices ) {
			lim::n_mesh::Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			mesh->vertices.push_back(temp);
		}

		// make triangles & edge
		mesh->indices.resize(triangles.size() * 3);

		int idxCount = 0;
		for( Triangle &tri : triangles ) {
			for( int i = 0; i < 3; i++ ) {
				mesh->indices[idxCount++] = tri.v[i];
				// 삼각형 노멀을 공유
				mesh->vertices[tri.v[i]].n = glm::vec3(tri.n.x, tri.n.y, tri.n.z);
				mesh->vertices[tri.v[i]].uv = glm::vec2(tri.uvs[i].x, tri.uvs[i].y);
			}
		}
		mesh->setupMesh();
	}

	lim::Mesh *simplifyMesh(lim::Mesh *mesh, float lived_pct
							, int version = 0, int agressiveness=7, bool verbose=true)
	{
		if( lived_pct > 1 || lived_pct < 0 ) {
			lim::Log::get().log("error : simplify percent range error");
			return nullptr;
		}
		updateGlobal(mesh);
		int target_count = (int)(triangles.size() * lived_pct);

		switch( version ) {
		case 0:
			simplify_mesh(target_count, agressiveness, verbose);
			break;
		case 1:
			simplify_mesh_max_threshold(target_count, verbose);
			break;
		case 2:
			simplify_mesh_lossless(verbose);
			break;
		}

		lim::Mesh *result = new lim::Mesh();
		result->replicateExtraData(*mesh);

		loadFromGlobal(result);
		return result;
	}

	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	// version 0 : original (use agressiveness)
	//		   1 : lossless
	//		   2 : max_consider_thresold
	//
	lim::Model *simplifyModel(const lim::Model *model, float lived_pct = 0.8f
							  , int version = 0, int agressiveness=7, bool verbose=true)
	{
		std::vector<lim::Mesh *> new_meshes;
		for( lim::Mesh *mesh : model->meshes ) {

			lim::Mesh *simp_mesh = simplifyMesh(mesh, lived_pct, version, agressiveness, verbose);

			if( simp_mesh != nullptr )
				new_meshes.push_back(simp_mesh);
		}
		lim::Model *result = new lim::Model(*model, new_meshes);
		return result;
	}
}

#endif