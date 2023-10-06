//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
// texture uniform sampler2d variable name rule
// map_Kd, map_Kd1 ...
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
#include <limbrary/utils.h>
#include <stack>

using namespace lim;

namespace
{
	void correctMatTexLink( const Material& src,  Material& dst
		, const std::vector<Texture*>& srcTexs, std::vector<Texture*>& dstTexs )
	{
		if( dst.map_Kd ) {
			dst.map_Kd = dstTexs[findIdx(srcTexs, src.map_Kd)];
		}
		if( dst.map_Ks ) {
			dst.map_Ks = dstTexs[findIdx(srcTexs, src.map_Ks)];
		}
		if( dst.map_Ka ) {
			dst.map_Ka = dstTexs[findIdx(srcTexs, src.map_Ka)];
		}
		if( dst.map_Ns ) {
			dst.map_Ns = dstTexs[findIdx(srcTexs, src.map_Ns)];
		}
		if( dst.map_Bump ) {
			dst.map_Bump = dstTexs[findIdx(srcTexs, src.map_Bump)];
		}
	}
}

namespace lim
{
	Model::Model(std::string_view _name)
		: name(_name)
	{
		default_material = &AssetLib::get().default_material;
	}
	Model::Model(const Model& src)
	{
		log::warn("Model is copied\n\n");
		name = src.name;
		path = src.path;

		position = src.position;
		orientation = src.orientation;
		scale = src.scale;
		pivot_mat = src.pivot_mat;
		model_mat = src.model_mat;
		default_material = src.default_material;

		my_textures.reserve(src.my_textures.size());
		for( Texture* srcTex : src.my_textures ) {
			my_textures.push_back( new Texture(*srcTex) ); // lvalue clone with copy consturctor
		}

		my_materials.reserve(src.my_materials.size());
		for( int i=0; i<src.my_materials.size(); i++ ) {
			my_materials.push_back( new Material(*src.my_materials[i]) ); // lvalue clone with copy consturctor
			correctMatTexLink( *src.my_materials[i], *my_materials[i], src.my_textures, my_textures);
		}

		my_meshes.reserve(src.my_meshes.size());
		for( Mesh* srcMs : src.my_meshes ) {
			my_meshes.push_back( new Mesh(*srcMs) ); // lvalue clone with copy consturctor
			if( srcMs->material == nullptr ) {
				continue;
			}
			int matIdx = findIdx(src.my_materials, srcMs->material);
			my_meshes.back()->material = my_materials[matIdx];
		}

		std::stack<const Node*> srcN;
		std::stack<Node*> dstN;
		srcN.push(&src.root);
		dstN.push(&root);
		while( srcN.size()>0 ) {
			const Node* orig = srcN.top(); srcN.pop();
			Node* copy = dstN.top(); dstN.pop();
			
			copy->meshes.reserve(orig->meshes.size());
			for(Mesh* mesh : orig->meshes) {
				copy->meshes.push_back(my_meshes[findIdx(src.my_meshes, mesh)]);
			}
			copy->childs.reserve(orig->childs.size());
			for(const Node& oriChild : orig->childs) {
				srcN.push(&oriChild);
				copy->childs.push_back({});
				dstN.push(&copy->childs.back());
			}
		}

		nr_vertices = src.nr_vertices;
		nr_triangles = src.nr_triangles;
		boundary_max = src.boundary_max;
		boundary_min = src.boundary_min;
		boundary_size = src.boundary_size;
		pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;
		ai_backup_flags = src.ai_backup_flags;
	}
	Model::Model(Model&& src) noexcept
	{
		*this = std::move(src);
	}
	Model& Model::operator=(Model&& src) noexcept
	{
		if( this==&src )
			return *this;
		releaseResource();

		name = std::move(src.name);
		path = std::move(src.path);
		position = src.position;
		scale = src.scale;
		pivot_mat = src.pivot_mat;
		model_mat = src.model_mat;
		
		root = src.root;
		default_material = src.default_material;
		my_materials = std::move(src.my_materials);
		my_textures = std::move(src.my_textures);
		my_meshes = std::move(src.my_meshes);

		nr_vertices = src.nr_vertices;
		nr_triangles = src.nr_triangles;
		boundary_size = src.boundary_size;
		boundary_min = src.boundary_min;
		boundary_max = src.boundary_max;
		pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;

		ai_backup_flags = src.ai_backup_flags;
		return *this;
	}
	Model::~Model() noexcept
	{
		releaseResource();
	}
	void Model::releaseResource()
	{
		for( Material* mat : my_materials )
			delete mat;
		for( Texture* tex : my_textures )
			delete tex;
		for( Mesh* ms : my_meshes )
			delete ms;
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
		boundary_max = glm::vec3(std::numeric_limits<float>::min());
		boundary_min = glm::vec3(std::numeric_limits<float>::max());
		boundary_size = glm::vec3(0);

		std::stack<Node*> nds;
		nds.push(&root);
		while(nds.size()>0)
		{
			Node* cur = nds.top(); nds.pop();
			for(Mesh* pMesh : cur->meshes) {
				for(glm::vec3& p : pMesh->poss) {
					boundary_max = glm::max(boundary_max, p);
					boundary_min = glm::min(boundary_min, p);
				}
				nr_vertices += pMesh->poss.size();
				nr_triangles += pMesh->tris.size();
			}
			for(Node& nd : cur->childs) {
				nds.push(&nd);
			}
		}
		boundary_size = boundary_max-boundary_min;
	}
	void Model::setPivot(const glm::vec3& pivot) 
	{
		pivot_mat = glm::translate(-pivot);
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

		pivoted_scaled_bottom_height = scale.y*boundary_size.y*0.5f;

		updateModelMat();
	}
}
