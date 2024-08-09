#include <limbrary/model_view/animator.h>
#include <limbrary/asset_lib.h>
#include <limbrary/application.h>
#include <limbrary/model_view/model.h>
#include <algorithm>

using namespace lim;
using namespace glm;


Animator::Animator() {
}
void Animator::init(const Model* md) {
    if( !md )
        return;
    md_data = md;
    mtx_Bones.resize(md_data->nr_bones);
    if( !AssetLib::get().app->update_hooks.isIn(this) ) {
        AssetLib::get().app->update_hooks[this] = [this](float dt) {update(dt);};
    }
}
Animator::~Animator() {
    clear();
    if( md_data ) {
        AssetLib::get().app->update_hooks.erase(this);
    } 
}
void Animator::clear() {
    nr_bone_nodes = 0;
    skeleton.clear();
    mtx_Bones.clear();
    cur_anim = nullptr;
    state = ST_STOP;
}
Animator& Animator::operator=(const Animator& src) {
    is_enabled = src.is_enabled;
    nr_bone_nodes = src.nr_bone_nodes;
    skeleton = src.skeleton;
    mtx_Bones = src.mtx_Bones;

    setAnim(src.cur_anim);
    init(src.md_data);
    elapsed_sec = src.elapsed_sec;
    duration_sec = src.duration_sec;
    is_loop = src.is_loop;
    return *this;
}

void Animator::setAnim(const Animation* anim) {
    if( !anim )
        return;
    is_enabled = true;
    cur_anim = anim;
    duration_sec = cur_anim->nr_ticks/cur_anim->ticks_per_sec;
    state = ST_STOP;
}
void Animator::setTimeline(const float pct, bool updateMenualy) {
    elapsed_sec = duration_sec*pct;
    if(updateMenualy) {
        State temp = state;
        state = ST_PLAY;
        update(0.f);
        state = temp;
    }
}
void Animator::play() {
    if( !cur_anim ) {
        log::err("no selected animation\n");
        assert(false);
    }
    if( state != ST_PAUSE ) {
        elapsed_sec = 0;
    }
    state = ST_PLAY;
}
void Animator::pause() {
    if( state == ST_STOP )
        return;
    state = ST_PAUSE;
}
void Animator::stop() {
    state = ST_STOP;
    elapsed_sec = 0;
}
void Animator::setUniformTo(const Program& prog) const {
    prog.setUniform("mtx_Bones", mtx_Bones);
}

template<typename T>
static void getKeyIdx(const std::vector<T>& keys, const int nrKeys, double curTick, int& idx, float& factor ) {
    for( int i=0; i<nrKeys; i++ ) {
        if( curTick < keys[i].tick ) {
            float dt = keys[i].tick - keys[i-1].tick;
            float diff = curTick - keys[i-1].tick;
            factor = diff / dt;
            idx = i;
            return;
        }
    }
}

static vec3 getInterpolatedValue( const std::map<float, vec3>& keys, float curTick ) {
    auto it = keys.lower_bound( curTick );
    float t2 = it->first;
    vec3 v2 = it->second;
    it--;
    float t1 = it->first;
    vec3 v1 = it->second;
    float dt = t2 - t1;
    float factor = (curTick - t1) / dt;
    return v1 + (v2 - v1) * factor;
}

static quat getInterpolatedValue( const std::map<float, quat>& keys, float curTick ) {
    auto it = keys.lower_bound( curTick );
    float t2 = it->first;
    quat v2 = it->second;
    it--;
    float t1 = it->first;
    quat v1 = it->second;
    float dt = t2 - t1;
    float factor = (curTick - t1) / dt;
    return slerp(v1, v2, factor);
}


void Animator::updateMtxBones() {
    BoneNode& rootBoneNode = skeleton[0];
    rootBoneNode.tf_model_space = rootBoneNode.tf.mtx;
    if( rootBoneNode.idx_bone == 0) {
        mtx_Bones[0] = rootBoneNode.tf_model_space * md_data->bone_offsets[0];
    }

    for(int i=1; i<nr_bone_nodes; i++) {
        BoneNode& curBoneNode = skeleton[i];
        BoneNode& parentBoneNode = skeleton[curBoneNode.idx_parent_bone_node];
        curBoneNode.tf_model_space = parentBoneNode.tf_model_space * curBoneNode.tf.mtx;
        int idxBone = curBoneNode.idx_bone;
        if( idxBone<0 ){
            continue;
        }
        else {
            mtx_Bones[idxBone] = curBoneNode.tf_model_space * md_data->bone_offsets[idxBone];
        }
    }
}
void Animator::update(float dt) {
    if( state!=ST_PLAY || !cur_anim )
        return;
    elapsed_sec += dt;
    if( elapsed_sec > duration_sec ) {
        if( is_loop ) {
            elapsed_sec = fmod(elapsed_sec, duration_sec);
        } 
        else {
            state = ST_STOP;
            return;
        }
    }
    cur_tick = elapsed_sec * cur_anim->ticks_per_sec;

    for( const Animation::Track& track : cur_anim->tracks ) {
        Transform& nodeTf = skeleton[track.idx_bone_node].tf;
        if( track.nr_poss>1 ) {
            nodeTf.pos = getInterpolatedValue(track.poss, cur_tick);
        } else {
            nodeTf.pos = track.poss.begin()->second;
        }
        if( track.nr_scales>1 ) {
            nodeTf.scale = getInterpolatedValue(track.scales, cur_tick);
        } else {
            nodeTf.scale = track.scales.begin()->second;
        }
        if( track.nr_oris>1 ) {
            nodeTf.ori = getInterpolatedValue(track.oris, cur_tick);
        } else {
            nodeTf.ori = track.oris.begin()->second;
        }
        nodeTf.update();
    }

    updateMtxBones();
}