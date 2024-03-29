/*
	2022-07-20 / im dong ye
	edit learnopengl code

	texture uniform sampler2d variable name rule
	map_BaseColor, map_Kd1 ...

	TODO list:
	0. transform, glm move constructor 리펙토링 필요함.
	1. export
	2. rigging
	3. not gl_static으로 실시간 vert변화
	4. width, height, depth 찾아서 -1~1공간으로 scaling
	5. load model 이 모델안에 있는데 따로 빼야될까
	6. 언제 어디서 업데이트해줘야하는지 규칙정하기
	7. reload normal map 외부로 빼기
*/

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
		if( dst.map_BaseColor ) {
			dst.map_BaseColor = dstTexs[findIdx(srcTexs, src.map_BaseColor)];
		}
		if( dst.map_Specular ) {
			dst.map_Specular = dstTexs[findIdx(srcTexs, src.map_Specular)];
		}
		if( dst.map_Bump ) {
			dst.map_Bump = dstTexs[findIdx(srcTexs, src.map_Bump)];
		}
		if( dst.map_AmbOcc ) {
			dst.map_AmbOcc = dstTexs[findIdx(srcTexs, src.map_AmbOcc)];
		}
		if( dst.map_Roughness ) {
			dst.map_Roughness = dstTexs[findIdx(srcTexs, src.map_Roughness)];
		}
		if( dst.map_Metalness ) {
			dst.map_Metalness = dstTexs[findIdx(srcTexs, src.map_Metalness)];
		}
		if( dst.map_Emission ) {
			dst.map_Emission = dstTexs[findIdx(srcTexs, src.map_Emission)];
		}
		if( dst.map_Opacity ) {
			dst.map_Opacity = dstTexs[findIdx(srcTexs, src.map_Opacity)];
		}
	}
}

namespace lim
{
	Model::Model(std::string_view _name)
		: Transform(), name(_name)
	{
		default_material = &AssetLib::get().default_material;
		updateModelMat();
	}
	Model::Model(const Model& src, bool makeRef)
		: Transform(src)
	{
		name = src.name;
		path = src.path;

		normalize_term = src.normalize_term;
		model_mat = src.model_mat;
		
		default_material = src.default_material;

		nr_vertices = src.nr_vertices;
		nr_triangles = src.nr_triangles;
		boundary_max = src.boundary_max;
		boundary_min = src.boundary_min;
		boundary_size = src.boundary_size;
		pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;
		ai_backup_flags = src.ai_backup_flags;

		
		if( makeRef ) 
		{
			std::stack<const Node*> srcNs;
			std::stack<Node*> dstNs;
			srcNs.push(&src.root);
			dstNs.push(&root);
			while( srcNs.size()>0 ) {
				const Node* srcN = srcNs.top(); srcNs.pop();
				Node* dstN = dstNs.top(); dstNs.pop();
				
				for( int i=0; i<srcN->getNrMesh(); i++ ) {
					auto [ms, mat] = srcN->getMeshWithMat(i);
					dstN->addMeshWithMat(ms, mat);
				}
				dstN->childs.reserve(srcN->childs.size());
				for(const Node& oriChild : srcN->childs) {
					srcNs.push(&oriChild);
					dstN->childs.push_back({});
					dstNs.push(&dstN->childs.back());
				}
			}
		}
		else // clone
		{
			log::warn("Model is copied\n\n");
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
			}

			std::stack<const Node*> srcNs;
			std::stack<Node*> dstNs;
			srcNs.push(&src.root);
			dstNs.push(&root);
			while( srcNs.size()>0 ) {
				const Node* srcN = srcNs.top(); srcNs.pop();
				Node* dstN = dstNs.top(); dstNs.pop();
				
				for( int i=0; i<srcN->getNrMesh(); i++ ) {
					auto [ms, mat] = srcN->getMeshWithMat(i);
					dstN->addMeshWithMat(my_meshes[findIdx(src.my_meshes, (Mesh*)ms)]
								, my_materials[findIdx(src.my_materials, (Material*)mat)]);
				}
				dstN->childs.reserve(srcN->childs.size());
				for(const Node& oriChild : srcN->childs) {
					srcNs.push(&oriChild);
					dstN->childs.push_back({});
					dstNs.push(&dstN->childs.back());
				}
			}
		}
	}
	Model::Model(Model&& src) noexcept
		: Transform(std::move(src))
	{
		name = src.name;
		path = src.path;

		normalize_term = src.normalize_term;
		model_mat = src.model_mat;
		
		root = src.root;

		default_material = src.default_material;
		my_materials = src.my_materials; 	src.my_materials.clear();
		my_textures = src.my_textures;		src.my_textures.clear();
		my_meshes = src.my_meshes;			src.my_meshes.clear();


		nr_vertices = src.nr_vertices;
		nr_triangles = src.nr_triangles;
		boundary_size = src.boundary_size;
		boundary_min = src.boundary_min;
		boundary_max = src.boundary_max;
		pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;
		ai_backup_flags = src.ai_backup_flags;
	}
	Model& Model::operator=(Model&& src) noexcept
	{
		if( this!=&src ) {
			releaseResource();
			Transform::operator=(src);

			name = src.name;
			path = src.path;

			normalize_term = src.normalize_term;
			model_mat = src.model_mat;
			
			root = src.root;

			default_material = src.default_material;
			my_materials = src.my_materials; 	src.my_materials.clear();
			my_textures = src.my_textures;		src.my_textures.clear();
			my_meshes = src.my_meshes;			src.my_meshes.clear();

			nr_vertices = src.nr_vertices;
			nr_triangles = src.nr_triangles;
			boundary_size = src.boundary_size;
			boundary_min = src.boundary_min;
			boundary_max = src.boundary_max;
			pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;
			ai_backup_flags = src.ai_backup_flags;
		}
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
		updateTransform();
		model_mat = transform * normalize_term.transform;
	}
	void Model::updateNrAndBoundary()
	{
		nr_vertices = 0;
		nr_triangles = 0;
		boundary_max = glm::vec3(std::numeric_limits<float>::min());
		boundary_min = glm::vec3(std::numeric_limits<float>::max());
		boundary_size = glm::vec3(0);

		std::stack<const Node*> nodeStack;
		nodeStack.push(&root);
		while(nodeStack.size()>0)
		{
			const Node& cur = *nodeStack.top(); nodeStack.pop();
			for( int i=0; i<cur.getNrMesh(); i++) {
				auto [ms, _] = cur.getMeshWithMat(i);
				for(const glm::vec3& p : ms->poss) {
					// Todo : transform
					boundary_max = glm::max(boundary_max, p);
					boundary_min = glm::min(boundary_min, p);
				}
				nr_vertices += ms->poss.size();
				nr_triangles += ms->tris.size();
			}
			for(const Node& nd : cur.childs) {
				nodeStack.push(&nd);
			}
		}
		boundary_size = boundary_max-boundary_min;
	}
	void Model::updateUnitScaleAndPivot()
	{
		if( nr_vertices==0 ) {
			updateNrAndBoundary();
		}
		constexpr float unit_length = 2.f;
		//float max_axis_length = glm::max(glm::max(boundary_size.x, boundary_size.y), boundary_size.z);
		float min_axis_length = glm::min(glm::min(boundary_size.x, boundary_size.y), boundary_size.z);
		float normScale = unit_length/min_axis_length;
		normalize_term.scale = glm::vec3(normScale);
		normalize_term.position = -(boundary_min + boundary_size*0.5f)*normScale;
		normalize_term.updateTransform();
		updateModelMat();

		pivoted_scaled_bottom_height = boundary_size.y*0.5f*normScale;
	}
}
