#include <limbrary/3d/model.h>
#include <limbrary/tools/log.h>
#include <limbrary/tools/glim.h>
#include <assimp/material.h>
#include <limbrary/tools/asset_lib.h>
#include <imgui.h>
#include <stack>

using namespace lim;

using glm::vec3;
using glm::vec4;
using glm::mat4;



RdNode::RdNode(const char* _name, const  RdNode* _parent, const Mesh* _ms, const Material* _mat)
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


RdNode& RdNode::addChild(const char* _name, const Mesh* _ms, const Material* _mat)
{
	childs.emplace_back(_name, this, _ms, _mat);
	return childs.back();
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
	if( visible && ms ) {
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

	own_meshes_in_view.clear();
	skinned_mesh_nodes.clear();
	if( src.own_meshes_in_view.empty() == false )
	{
		skinned_mesh_nodes.clear();
		int nrSkMs = src.skinned_mesh_nodes.size();
		skinned_mesh_nodes.reserve(nrSkMs);
		// copy meshes
		own_meshes_in_view = src.own_meshes_in_view;
		int nrMs = src.own_meshes_in_view.size();
		
		root.dfsAll([&](RdNode& nd) {
			for( int i=0; i<nrMs; i++ ) {
				if( nd.ms==src.own_meshes_in_view[i].raw ) {
					for( int j=0; j<nrSkMs; j++ ) {
						if( nd.ms==src.skinned_mesh_nodes[j]->ms ) {
							skinned_mesh_nodes[j] = &nd;
						}
					}
					nd.ms = own_meshes_in_view[i].raw;
				}
			}
			return true;
		});

		assert(skinned_mesh_nodes.size()==nrSkMs);
	}

	tf_prev = src.tf_prev;
	src_md = src.src_md;
	return *this;
}


void ModelView::clear() noexcept
{
	root.clear();
	own_animator.clear();
	own_meshes_in_view.clear();
	skinned_mesh_nodes.clear();
	tf_prev = nullptr;
	src_md = nullptr;
}

mat4 ModelView::getLocalToMeshMtx(const Mesh* target) const
{
	bool isFindMs = false;
	mat4 rst = mat4(1.0f);
	root.dfsAll([&](const RdNode& nd) {
		if( target == nd.ms ) {
			rst = nd.mtx_global;
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
	for( int i=0; i<src_md->depth_of_bone_root_in_rdtree; i++ ) {
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



Model::Model(const char* _name): name(_name)
{
	src_md = this;
}
Model::~Model()
{
	clear();
}
void Model::clear() noexcept
{
	ModelView::clear();
	src_md = this;

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
		if( nd.ms || nd.mat ) {
			nd.mat = mat;
		}
		return true;
	});
}


static void correctMatTexLink( 
	Material& dst,
	const Material& src,
	std::vector<OwnPtr<Texture>>& dstTexs,
	const std::vector<OwnPtr<Texture>>& srcTexs 
) 
{
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
static void matchRdTree(RdNode& dst, const RdNode& src, const Model& dstMd, const Model& srcMd)
{
	if( src.ms ) {
		const int msIdx = findIdx(srcMd.own_meshes, (Mesh*)src.ms);
		const int matIdx = findIdx(srcMd.own_materials, (Material*)src.mat);
		dst.ms = dstMd.own_meshes[msIdx].raw;
		dst.mat = dstMd.own_materials[matIdx].raw;
	}

	const int nrChilds = src.childs.size();
	for( int i=0 ; i<nrChilds; i++ ) {
		matchRdTree(dst.childs[i], src.childs[i], dstMd, srcMd);
	}
}
/**************
 * copy model
 **************/
Model::Model(const Model& src)
{	// 복사생성자는 자동으로 부모 복사 생성자 호출하지 않음.
	operator=(src);
}
Model& Model::operator=(const Model& src)
{
	log::warn("copy model data\n");
	clear();
	ModelView::operator=(src);

	src_md = this;
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

	return *this;
}