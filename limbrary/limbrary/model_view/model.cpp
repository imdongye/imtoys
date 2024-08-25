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
#include <limbrary/tools/log.h>
#include <limbrary/tools/glim.h>
#include <assimp/material.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/general.h>
#include <imgui.h>
#include <stack>

using namespace lim;

using glm::vec3;
using glm::vec4;
using glm::mat4;



RdNode::RdNode(std::string_view _name, const  RdNode* _parent, const Mesh* _ms, const Material* _mat)
	: name(_name), parent(_parent), ms(_ms), mat(_mat)
{
	if( parent ) {
		mtx_global = parent->mtx_global;
	}
}
RdNode::RdNode(const RdNode& src)
{
	operator=(src);
}
RdNode& RdNode::operator=(const RdNode& src)
{
	name = src.name;
	tf = src.tf;
	mtx_global = src.mtx_global;
	parent = src.parent; // override next time
	childs = src.childs;
	// recursivly bottom to top
	for( auto& child : childs ) {
		child.parent = this;
	}

	enabled = src.enabled;
	visible = src.visible;
	is_identity_mtx = src.is_identity_mtx;

	ms = src.ms;
	mat = src.mat;
	return *this;
}
RdNode::RdNode(RdNode&& src) noexcept
{
	operator=(std::move(src));
}
RdNode& RdNode::operator=(RdNode&& src) noexcept
{
	name = std::move(src.name);
	tf = src.tf;
	mtx_global = src.mtx_global;
	parent = src.parent; // override next time
	childs.clear();
	childs = std::move(src.childs);
	// recursivly bottom to top
	for( auto& child : childs ) {
		child.parent = this;
	}

	enabled = src.enabled;
	visible = src.visible;
	is_identity_mtx = src.is_identity_mtx;

	ms = src.ms;
	mat = src.mat;
	return *this;
}

void RdNode::clear() noexcept
{
	parent = nullptr;
	childs.clear();
	ms = nullptr;
	mat = nullptr;
}


void RdNode::addChild(std::string_view _name, const Mesh* _ms, const Material* _mat)
{
	childs.emplace_back(_name, this, _ms, _mat);
}


void RdNode::updateGlobalTransform(mat4 prevTf) {
	if( is_local_is_global ) {
		mtx_global = tf.mtx;
	}
	else if( is_identity_mtx ) {
		mtx_global = prevTf;
	}
	else {
		mtx_global = prevTf * tf.mtx;
	}
	for( RdNode& child : childs ) {
		child.updateGlobalTransform(mtx_global);
	}
}

void RdNode::dfsRender(std::function<void(const Mesh* ms, const Material* mat, const glm::mat4& mtxGlobal)> callback) const
{
	if( enabled == false ) {
		return;
	}
	if( visible ) {
		callback(ms, mat, mtx_global);
	}
	for( const RdNode& child : childs ) {
		child.dfsRender(callback);
	}
}
void RdNode::dfsAll(std::function<bool(RdNode& nd)> callback)
{
	if( callback(*this) == false ) {
		return;
	}
	for( RdNode& child : childs ) {
		child.dfsAll(callback);
	}
}
void RdNode::dfsAll(std::function<bool(const RdNode& nd)> callback) const
{
	if( callback(*this) == false ) {
		return;
	}
	for( const RdNode& child : childs ) {
		child.dfsAll(callback);
	}
}












ModelView::ModelView()
	: root("root", nullptr, nullptr, nullptr)
{
}
ModelView::~ModelView()
{
}
ModelView::ModelView(const ModelView& src)
	: ModelView()
{
	operator=(src);
}
ModelView& ModelView::operator=(const ModelView& src) 
{
	root = src.root;
	own_animator = src.own_animator;

	own_meshes.clear();
	skinned_mesh_nodes.clear();
	if( src.own_meshes.empty() == false )
	{
		skinned_mesh_nodes.clear();
		int nrSkMs = src.skinned_mesh_nodes.size();
		skinned_mesh_nodes.reserve(nrSkMs);
		// copy meshes
		own_meshes = src.own_meshes;
		int nrMs = src.own_meshes.size();
		
		root.dfsAll([&](RdNode& nd) {
			for( int i=0; i<nrMs; i++ ) {
				if( nd.ms==src.own_meshes[i].raw ) {
					for( int j=0; j<nrSkMs; j++ ) {
						if( nd.ms==src.skinned_mesh_nodes[j]->ms ) {
							skinned_mesh_nodes[j] = &nd;
						}
					}
					nd.ms = own_meshes[i].raw;
				}
			}
			return true;
		});

		assert(skinned_mesh_nodes.size()==nrSkMs);
	}

	tf_prev = src.tf_prev;
	md_data = src.md_data;
	return *this;
}


void ModelView::clear() noexcept
{
	root.clear();
	own_animator.clear();
	own_meshes.clear();
	skinned_mesh_nodes.clear();
	tf_prev = nullptr;
	md_data = nullptr;
}

mat4 ModelView::getLocalToMeshMtx(const Mesh* target) const
{
	bool isFindMs = false;
	mat4 rst = mat4(1.0f);
	root.dfsAll([&](const RdNode& nd) {
		if( target == nd.ms ) {
			rst = nd.tf.mtx;
			isFindMs = true;
			return false;
		}
		return true;
	});
	assert(isFindMs);
	return rst;
}

mat4 ModelView::getLocalToBoneRootMtx() const
{
	const RdNode* curNode = &root;
	for( int i=0; i<md_data->depth_of_bone_root_in_rdtree; i++ ) {
		assert(curNode->childs.size()==1);
		curNode = &curNode->childs[0];
	}
	return curNode->mtx_global;
}

vec3 ModelView::getBoneWorldPos(int boneNodeIdx) const
{
    mat4 toBoneRoot = getLocalToBoneRootMtx();
	vec3 rst = vec3( toBoneRoot * own_animator->bones[boneNodeIdx].mtx_model_space * vec4(0,0,0,1) );
    return rst;
}



Model::Model(std::string_view _name): name(_name)
{
	md_data = this;
}
Model::~Model()
{
	clear();
}
void Model::clear() noexcept
{
	ModelView::clear();
	own_materials.clear();
	own_textures.clear();
	own_meshes.clear();

	name_to_weighted_bone_idx.clear();
	weighted_bone_offsets.clear();
	animations.clear();
}



Material* Model::addOwn(Material* md)
{
	own_materials.push_back(md);
	return md;
}
Texture* Model::addOwn(Texture* tex)
{
	own_textures.push_back(tex);
	return tex;
}
Mesh* Model::addOwn(Mesh* ms)
{
	own_meshes.push_back(ms);
	return ms;
}



void Model::setProgToAllMat(const Program* prog)
{
	for( auto& mat : own_materials ) {
		mat->prog = prog;
	}
}
void Model::setSetProgToAllMat(std::function<void(const Program&)> setProg)
{
	for( auto& mat : own_materials ) {
		mat->set_prog = setProg;
	}
}
void Model::setSameMat(const Material* mat)
{
	root.dfsAll([&](RdNode& nd) {
		nd.mat = mat;
		return true;
	});
}


static void correctMatTexLink( Material& dst, const Material& src
	, std::vector<OwnPtr<Texture>>& dstTexs, const std::vector<OwnPtr<Texture>>& srcTexs ) {
	if( dst.map_ColorBase ) {
		dst.map_ColorBase = dstTexs[findIdx(srcTexs, src.map_ColorBase)].raw;
	}
	if( dst.map_Specular ) {
		dst.map_Specular = dstTexs[findIdx(srcTexs, src.map_Specular)].raw;
	}
	if( dst.map_Bump ) {
		dst.map_Bump = dstTexs[findIdx(srcTexs, src.map_Bump)].raw;
	}
	if( dst.map_AmbOcc ) {
		dst.map_AmbOcc = dstTexs[findIdx(srcTexs, src.map_AmbOcc)].raw;
	}
	if( dst.map_Roughness ) {
		dst.map_Roughness = dstTexs[findIdx(srcTexs, src.map_Roughness)].raw;
	}
	if( dst.map_Metalness ) {
		dst.map_Metalness = dstTexs[findIdx(srcTexs, src.map_Metalness)].raw;
	}
	if( dst.map_Emission ) {
		dst.map_Emission = dstTexs[findIdx(srcTexs, src.map_Emission)].raw;
	}
	if( dst.map_Opacity ) {
		dst.map_Opacity = dstTexs[findIdx(srcTexs, src.map_Opacity)].raw;
	}
}
static void matchRdTree(RdNode& dst, const RdNode& src, const Model& dstMd, const Model& srcMd) {
	const int msIdx = findIdx(srcMd.own_meshes, (Mesh*)src.ms);
	const int matIdx = findIdx(srcMd.own_materials, (Material*)src.mat);
	dst.ms = dstMd.own_meshes[msIdx].raw;
	dst.mat = dstMd.own_materials[matIdx].raw;

	const int nrChilds = src.childs.size();
	for( int i=0 ; i<nrChilds; i++ ) {
		matchRdTree(dst.childs[i], src.childs[i], dstMd, srcMd);
	}
}
void Model::copyFrom(const Model& src)
{
	clear();
	ModelView::operator=(src);

	md_data = this;
	name = src.name;
	path = src.path;

	own_materials = src.own_materials;
	own_textures = src.own_textures;
	own_meshes = src.own_meshes;

	for( int i=0; i<src.own_materials.size(); i++ ) {
		correctMatTexLink( *own_materials[i], *src.own_materials[i], own_textures, src.own_textures);
	}

	// root = src.root; in ModelView Copy
	matchRdTree(root, src.root, *this, src);


	total_verts = src.total_verts;
	total_tris = src.total_tris;
	boundary_max = src.boundary_max;
	boundary_min = src.boundary_min;
	boundary_size = src.boundary_size;
	ai_backup_flags = src.ai_backup_flags;

	
	depth_of_bone_root_in_rdtree = src.depth_of_bone_root_in_rdtree;
	nr_weighted_bones = src.nr_weighted_bones;
	name_to_weighted_bone_idx = src.name_to_weighted_bone_idx;
	weighted_bone_offsets = src.weighted_bone_offsets;
	animations = src.animations;
}

