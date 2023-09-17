#include "simplify.h"
#include "fqms.h"
#include <limbrary/log.h>
#include <glm/glm.hpp>

using namespace fqms;

namespace
{
	bool hasNormal, hasUv;

    void updateGlobal(lim::Mesh *mesh)
	{

		vertices.clear();
		for( const glm::vec3& p : mesh->poss ) {
			Vertex temp;
			temp.p.x = p.x;
			temp.p.y = p.y;
			temp.p.z = p.z;
			vertices.push_back(temp);
		}

		// make triangles & edge
		int triIdx = 0;
		triangles.clear();
		triangles.resize(mesh->tris.size());
		for( glm::uvec3& from : mesh->tris ) {
			Triangle& to = triangles[triIdx++];
			to.v[0] = from.x;
			to.v[1] = from.y;
			to.v[2] = from.z;
			hasNormal = mesh->nors.size()>0;
			if(hasNormal) {
				glm::vec3 faceN = glm::normalize(mesh->nors[from.x]+mesh->nors[from.y]+mesh->nors[from.z]);
				to.n = vec3f(faceN.x, faceN.y, faceN.z);
			}
			hasUv = mesh->uvs.size()>0;
			if(hasUv) {
				to.uvs[0]  = vec3f(mesh->uvs[from.x].x, mesh->uvs[from.x].y, 1);
				to.uvs[1]  = vec3f(mesh->uvs[from.y].x, mesh->uvs[from.y].y, 1);
				to.uvs[2]  = vec3f(mesh->uvs[from.z].x, mesh->uvs[from.z].y, 1);
			}
		}
	}
	void loadFromGlobal(lim::Mesh *mesh)
	{
		mesh->poss.clear();
		mesh->nors.clear();

		for( Vertex &v : vertices ) {
            mesh->poss.push_back({v.p.x, v.p.y, v.p.z});
			if(hasNormal)
				mesh->nors.push_back({});
			if(hasUv)
				mesh->nors.push_back({});
		}

		// make triangles & edge
		mesh->tris.resize(triangles.size());

		for( Triangle &tri : triangles ) {
            mesh->tris.push_back({});
			for( int i = 0; i < 3; i++ ) {
				mesh->tris.back()[i] = tri.v[i];
				// 삼각형 노멀을 공유
				if(hasNormal)
					mesh->nors[tri.v[i]] = glm::vec3(tri.n.x, tri.n.y, tri.n.z);
				if(hasUv)
					mesh->uvs[tri.v[i]] = glm::vec2(tri.uvs[i].x, tri.uvs[i].y);
			}
		}
		mesh->initGL();
	}
}

namespace fqms
{
	void simplifyMesh(lim::Mesh *mesh, float lived_pct, int version, int agressiveness, bool verbose)
	{
		if( lived_pct > 1 || lived_pct < 0 ) {
			lim::log::err("simplify percent range error");
			return;
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

		loadFromGlobal(mesh);
	}

	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	// version 0 : original (use agressiveness)
	//		   1 : lossless
	//		   2 : max_consider_thresold
	//
	void simplifyModel(const lim::Model *model, float lived_pct, int version, int agressiveness, bool verbose)
	{
		for( lim::Mesh *mesh : model->meshes ) {
			simplifyMesh(mesh, lived_pct, version, agressiveness, verbose);
		}
	}
}