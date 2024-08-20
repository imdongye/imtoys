#include "simplify.h"
#include "fqms.h"
#include <limbrary/log.h>
#include <glm/glm.hpp>

using namespace fqms;

namespace
{
	bool hasNormal, hasUv;

    void updateGlobal(lim::Mesh& mesh)
	{

		Simplify::vertices.clear();
		for( const glm::vec3& p : mesh.poss ) {
			Simplify::Vertex temp;
			temp.p.x = p.x;
			temp.p.y = p.y;
			temp.p.z = p.z;
			Simplify::vertices.push_back(temp);
		}

		// make triangles & edge
		int triIdx = 0;
		Simplify::triangles.clear();
		Simplify::triangles.resize(mesh.tris.size());
		for( glm::ivec3& from : mesh.tris ) {
			Simplify::Triangle& to = Simplify::triangles[triIdx++];
			to.v[0] = from.x;
			to.v[1] = from.y;
			to.v[2] = from.z;
			hasNormal = mesh.nors.size()>0;
			if( hasNormal ) {
				glm::vec3 faceN = glm::normalize(mesh.nors[from.x]+mesh.nors[from.y]+mesh.nors[from.z]);
				to.n = vec3f(faceN.x, faceN.y, faceN.z);
			}
			hasUv = mesh.uvs.size()>0;
			if( hasUv ) {
				to.attr |= Simplify::ATRB_TEXCOORD;
				to.uvs[0]  = vec3f(mesh.uvs[from.x].x, mesh.uvs[from.x].y, 1);
				to.uvs[1]  = vec3f(mesh.uvs[from.y].x, mesh.uvs[from.y].y, 1);
				to.uvs[2]  = vec3f(mesh.uvs[from.z].x, mesh.uvs[from.z].y, 1);
			}
		}
	}
	void loadFromGlobal(lim::Mesh& mesh)
	{
		mesh.poss.resize(Simplify::vertices.size());
		if( hasNormal )
			mesh.nors.resize(Simplify::vertices.size());
		if( hasUv )
			mesh.uvs.resize(Simplify::vertices.size());

		for( int i=0; i<Simplify::vertices.size(); i++ ) {
			Simplify::Vertex& v = Simplify::vertices[i];
            mesh.poss[i] = {v.p.x, v.p.y, v.p.z};
		}

		// make triangles & edge
		mesh.tris.resize( Simplify::triangles.size() );

		for( int i=0; i<Simplify::triangles.size(); i++ ) {
			Simplify::Triangle& tri = Simplify::triangles[i];
			for( int j = 0; j < 3; j++ ) {
				mesh.tris[i][j] = tri.v[j];
				// 삼각형 노멀을 공유
				if(hasNormal)
					mesh.nors[tri.v[j]] = glm::vec3(tri.n.x, tri.n.y, tri.n.z);
				if(hasUv)
					mesh.uvs[tri.v[j]] = glm::vec2(tri.uvs[j].x, tri.uvs[j].y);
			}
		}
		mesh.initGL();
	}
}

namespace lim
{
	void simplifyMesh(lim::Mesh& mesh, float lived_pct, int version, int agressiveness, bool verbose)
	{
		if( lived_pct > 1 || lived_pct < 0 ) {
			lim::log::err("simplify percent range error");
			return;
		}
		updateGlobal(mesh);
		int target_count = (int)(Simplify::triangles.size() * lived_pct);

		switch( version ) {
		case 0:
			Simplify::simplify_mesh(target_count, agressiveness, verbose);
			break;
		case 1:
			Simplify::simplify_mesh_max_threshold(target_count, verbose);
			break;
		case 2:
			Simplify::simplify_mesh_lossless(verbose);
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
	void simplifyModel(lim::Model& model, float lived_pct, int version, int agressiveness, bool verbose)
	{
		model.total_verts = 0;
		model.total_tris = 0;
		for( lim::Mesh *mesh : model.own_meshes ) {
			simplifyMesh(*mesh, lived_pct, version, agressiveness, verbose);
			model.total_verts += mesh->poss.size();
			model.total_tris += mesh->tris.size();
		}
	}
}