//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
// texture uniform sampler2d variable name rule
// map_Kd0, map_Kd1 ...
// 
//  TODO list:
//  1. export
//  2. rigging
//  3. not gl_static으로 실시간 vert변화
//  4. width, height, depth 찾아서 -1~1공간으로 scaling
//  5. load model 이 모델안에 있는데 따로 빼야될까
//  6. 언제 어디서 업데이트해줘야하는지 규칙정하기
//	7. reload normal map 외부로 빼기
// 
//
#include <limbrary/model_view/model.h>
#include <limbrary/log.h>
#include <glm/gtx/transform.hpp>
#include <assimp/material.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace lim;

namespace
{
	void correctMatTexLink( Material& to, const Material& from, 
		const std::vector<Texture*>& toTexs, const std::vector<Texture*>& fromTexs ) {
		if( to.map_Kd ) {
			to.map_Kd = toTexs[std::distance(fromTexs.begin(), std::find(fromTexs.begin(), fromTexs.end(), from.map_Kd))];
		}
		if( to.map_Ks ) {
			to.map_Ks = toTexs[std::distance(fromTexs.begin(), std::find(fromTexs.begin(), fromTexs.end(), from.map_Ks))];
		}
		if( to.map_Ka ) {
			to.map_Ka = toTexs[std::distance(fromTexs.begin(), std::find(fromTexs.begin(), fromTexs.end(), from.map_Ka))];
		}
		if( to.map_Ns ) {
			to.map_Ns = toTexs[std::distance(fromTexs.begin(), std::find(fromTexs.begin(), fromTexs.end(), from.map_Ns))];
		}
		if( to.map_Bump ) {
			to.map_Bump = toTexs[std::distance(fromTexs.begin(), std::find(fromTexs.begin(), fromTexs.end(), from.map_Bump))];
		}
	}
}

namespace lim
{
	Model::Model()
	{
		default_mat = AssetLib::get().default_mat;
	}
	Model::~Model()
	{
		for( auto mat : materials )
			delete mat;
		for( auto tex : textures_loaded )
			delete tex;
		for( auto mesh : meshes )
			delete mesh;
	}
	Model* Model::clone()
	{
		Model* rst = new Model();
		Model& dup = *rst;

		dup.name = name;
		dup.path = path;

		dup.position = position;
		dup.orientation = orientation;
		dup.scale = scale;
		dup.pivot_mat = pivot_mat;
		dup.model_mat = model_mat;
	
		dup.default_mat = default_mat;


		dup.textures_loaded.reserve(textures_loaded.size());
		dup.materials.reserve(materials.size());
		for( Texture* tex : textures_loaded ) {
			dup.textures_loaded.push_back( tex->clone() );
		}
		for( Material* mat : materials ) {
			dup.materials.push_back( new Material(*mat) );
			correctMatTexLink(*dup.materials.back(), *mat, dup.textures_loaded, textures_loaded);
		}

		dup.meshes.reserve(meshes.size());
		for( Mesh* ms : meshes ) {
			dup.meshes.push_back( ms->clone() );
			if( ms->material == nullptr ) {
				dup.meshes.back()->material = nullptr;
				continue;
			}
			int matIdx = std::distance(materials.begin(), std::find(materials.begin(), materials.end(), ms->material));
			dup.meshes.back()->material = dup.materials[matIdx];
		}

		std::stack<Node*> origs;
		std::stack<Node*> copys;
		origs.push(&root);
		copys.push(&dup.root);
		while( origs.size()>0 ) {
			Node* orig = origs.top(); origs.pop();
			Node* copy = copys.top(); copys.pop();
			
			copy->meshes.reserve(orig->meshes.size());
			for(Mesh* mesh : orig->meshes) {
				GLuint idxOfMesh = std::distance(meshes.begin(), std::find(meshes.begin(), meshes.end(), mesh));
				copy->meshes.push_back(dup.meshes[idxOfMesh]);
			}
			copy->childs.reserve(orig->childs.size());
			for(Node& oriChild : orig->childs) {
				origs.push(&oriChild);
				copys.push(new Node());
				copy->childs.push_back(copys.top());
			}
		}

		dup.nr_vertices = nr_vertices;
		dup.nr_triangles = nr_triangles;
		dup.boundary_max = boundary_max;
		dup.boundary_min = boundary_min;
		dup.boundary_size = boundary_size;
		dup.pivoted_scaled_bottom_height = pivoted_scaled_bottom_height;
		
		return rst;
	}
	void Model::updateModelMat()
	{
		glm::mat4 translateMat = glm::translate(position);
		glm::mat4 scaleMat = glm::scale(scale);
		glm::mat4 rotateMat = glm::toMat4(orientation);
		model_mat = translateMat * rotateMat * scaleMat * pivot_mat;
	}
	void Model::updateNrAndBoundary()
	{
		nr_vertices = 0;
		nr_triangles = 0;
		if( meshes.size()>0 && meshes[0]->poss.size()>0 ) {
			boundary_max = meshes[0]->poss[0];
			boundary_min = meshes[0]->poss[0];
		}
		else {
			boundary_max = glm::vec3(-1);
			boundary_min = glm::vec3(-1);
			log::err("there's no mesh when updateNrBoundary\n");
			return;
		}

		for(Mesh* pMesh : meshes) {
			const Mesh& m = *pMesh;
			nr_vertices += m.poss.size();
			for(glm::vec3 p : m.poss) {
				boundary_max = glm::max(boundary_max, p);
				boundary_min = glm::min(boundary_min, p);
			}
			nr_triangles += m.tris.size();
		}
		boundary_size = boundary_max-boundary_min;
	}
	void Model::updateUnitScaleAndPivot()
	{
		if( nr_vertices==0 ) {
			updateNrAndBoundary();
		}
		constexpr float unit_length = 2.f;
		float max_axis_length = glm::max(glm::max(boundary_size.x, boundary_size.y), boundary_size.z);
		scale = glm::vec3(unit_length/max_axis_length);

		setPivot(boundary_min + boundary_size*0.5f);

		updateModelMat();
	}
	void Model::setPivot(const glm::vec3& pivot) 
	{
		pivot_mat = glm::translate(-pivot);
		pivoted_scaled_bottom_height = -(scale.y*boundary_size.y*0.5f);
	}
}
