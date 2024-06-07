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



void RdNode::addMsMat(const Mesh* ms, const Material* mat) {
	meshs_mats.push_back({ms, mat});
}
RdNode* RdNode::makeChild(std::string_view _name) {
	childs.push_back(RdNode());
	childs.back().name = _name;
	return &childs.back();
}
void RdNode::treversal(std::function<void(const Mesh* ms, const Material* mat, const glm::mat4& transform)> callback
				, const glm::mat4& prevTransform ) const
{
	const glm::mat4 curTf = prevTransform*transform.mtx;

	for( auto [ms, mat] : meshs_mats ) {
		callback(ms, mat, curTf);
	}
	for( const RdNode& child : childs ) {
		child.treversal(callback, curTf);
	}
}
void RdNode::clear() {
	meshs_mats.clear();
	childs.clear();
}





Model::Model(std::string_view _name)
	: name(_name)
{
	tf = &root.transform;
}
Model::~Model()
{
	clear();
}
void Model::clear()
{
	for( Material* mat : own_materials )
		delete mat;
	for( Texture* tex : own_textures )
		delete tex;
	for( Mesh* ms : own_meshes )
		delete ms;

	own_materials.clear();
	own_textures.clear();
	own_meshes.clear();
	root.clear();

	tf = &root.transform;
	tf_normalized = nullptr;
}





static void correctMatTexLink( const Material& src,  Material& dst
	, const std::vector<Texture*>& srcTexs, std::vector<Texture*>& dstTexs )
{
	if( dst.map_ColorBase ) {
		dst.map_ColorBase = dstTexs[findIdx(srcTexs, src.map_ColorBase)];
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
static void recursiveCopyTree(const RdNode& src, RdNode& dst) {
	dst.name = src.name;
	dst.transform = src.transform;
	dst.meshs_mats = src.meshs_mats;
	
	dst.childs.reserve(src.childs.size());
	for( const RdNode& child : src.childs ) {
		dst.childs.push_back({});
		recursiveCopyTree(child, dst.childs.back());
	}
}
static void recursiveCopyAndMatchTree(const RdNode& src, RdNode& dst, const Model& srcMd, const Model& dstMd) {
	dst.name = src.name;
	dst.transform = src.transform;
	dst.meshs_mats.reserve(src.meshs_mats.size());
	for( auto& [srcMs, srcMat] : dst.meshs_mats ) {
		int msIdx = findIdx(srcMd.own_meshes, (Mesh*)srcMs);
		int matIdx = findIdx(srcMd.own_materials, (Material*)srcMat);
		dst.meshs_mats.push_back({dstMd.own_meshes[msIdx], dstMd.own_materials[matIdx]});
	}
	
	dst.childs.reserve(src.childs.size());
	for( const RdNode& child : src.childs ) {
		dst.childs.push_back({});
		recursiveCopyAndMatchTree(child, dst.childs.back(), srcMd, dstMd);
	}
}
void Model::copyFrom(const Model& src, bool makeRef)
{
	clear();
	name = src.name;
	path = src.path;

	nr_vertices = src.nr_vertices;
	nr_triangles = src.nr_triangles;
	boundary_max = src.boundary_max;
	boundary_min = src.boundary_min;
	boundary_size = src.boundary_size;
	pivoted_scaled_bottom_height = src.pivoted_scaled_bottom_height;
	ai_backup_flags = src.ai_backup_flags;

	if( makeRef ) 
	{
		recursiveCopyTree(src.root, root);
	}
	else // clone
	{
		log::warn("Model is copied\n\n");
		own_textures.reserve(src.own_textures.size());
		for( Texture* srcTex : src.own_textures ) {
			own_textures.push_back( new Texture(*srcTex) );
		}

		own_materials.reserve(src.own_materials.size());
		for( int i=0; i<src.own_materials.size(); i++ ) {
			own_materials.push_back( new Material(*src.own_materials[i]) );
			correctMatTexLink( *src.own_materials[i], *own_materials[i], src.own_textures, own_textures);
		}

		own_meshes.reserve(src.own_meshes.size());
		for( Mesh* srcMs : src.own_meshes ) {
			own_meshes.push_back( new Mesh(*srcMs) );
		}

		recursiveCopyAndMatchTree(src.root, root, src, *this);
	}
	if( src.tf == &src.root.transform ) {
		tf = &root.transform;
		tf_normalized = nullptr;
	} else {
		tf = &root.childs[0].transform;
		tf_normalized = &root.transform;
	}
}





Material* Model::addOwn(Material* md) {
	own_materials.push_back(md);
	return md;
}
Texture* Model::addOwn(Texture* tex) {
	own_textures.push_back(tex);
	return tex;
}
Mesh* Model::addOwn(Mesh* ms) {
	own_meshes.push_back(ms);
	return ms;
}


void Model::setProgToAllMat(const Program* prog)
{
	for( Material* mat : own_materials ) {
		mat->prog = prog;
	}
}
void Model::setSetProgToAllMat(std::function<void(const Program&)> setProg)
{
	for( Material* mat : own_materials ) {
		mat->set_prog = setProg;
	}
}


void Model::updateNrAndBoundary()
{
	nr_vertices = 0;
	nr_triangles = 0;
	boundary_max = glm::vec3(std::numeric_limits<float>::min());
	boundary_min = glm::vec3(std::numeric_limits<float>::max());
	boundary_size = glm::vec3(0);

	root.treversal([&](const Mesh* ms, const Material* mat, const glm::mat4& transform) {
		nr_vertices += ms->poss.size();
		nr_triangles += ms->tris.size();
		for(glm::vec3 p : ms->poss) {
			p = transform * glm::vec4(p, 1);
			boundary_max = glm::max(boundary_max, p);
			boundary_min = glm::min(boundary_min, p);
		}
	});
	boundary_size = boundary_max-boundary_min;
}
void Model::updateUnitScaleAndPivot()
{
	constexpr float unit_length = 2.f;
	float max_axis_length = glm::max(glm::max(boundary_size.x, boundary_size.y), boundary_size.z);
	float min_axis_length = glm::min(glm::min(boundary_size.x, boundary_size.y), boundary_size.z);
	float normScale = unit_length/max_axis_length;

	if( !tf_normalized ) {
		RdNode real = root;
		root.meshs_mats.clear();
		root.childs.clear();
		root.childs.push_back(real);
		root.childs.back().name = "pivot";
		tf_normalized = &(root.childs.back().transform);
		*tf_normalized = Transform();
	}

	tf_normalized->scale = glm::vec3(normScale);
	tf_normalized->pos = -(boundary_min + boundary_size*0.5f)*normScale;
	tf_normalized->update();

	pivoted_scaled_bottom_height = boundary_size.y*0.5f*normScale;
	// pivoted_scaled_bottom_height = 0;
}