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
    state = State::STOP;
}
Animator& Animator::operator=(const Animator& src) {
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
    cur_anim = anim;
    duration_sec = cur_anim->nr_ticks/cur_anim->ticks_per_sec;
    state = State::STOP;
}
void Animator::setTimeline(const float pct, bool updateMenualy) {
    elapsed_sec = duration_sec*pct;
    if(updateMenualy) {
        State temp = state;
        state = State::PLAY;
        update(0.f);
        state = temp;
    }
}
void Animator::play() {
    if( !cur_anim ) {
        log::err("no selected animation\n");
        assert(false);
    }
    if( state != State::PAUSE ) {
        elapsed_sec = 0;
    }
    state = State::PLAY;
}
void Animator::pause() {
    if( state == State::STOP )
        return;
    state = State::PAUSE;
}
void Animator::stop() {
    state = State::STOP;
    elapsed_sec = 0;
}
void Animator::setUniformTo(const Program& prog) const {
    if( state == State::STOP ) {
        prog.setUniform("is_Skinned", false);
    } else {
        prog.setUniform("is_Skinned", true);
        prog.setUniform("mtx_Bones", mtx_Bones);
    }
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
    if( state!=State::PLAY || !cur_anim )
        return;
    elapsed_sec += dt;
    if( elapsed_sec > duration_sec ) {
        if( is_loop ) {
            elapsed_sec = fmod(elapsed_sec, duration_sec);
        } 
        else {
            state = State::STOP;
            return;
        }
    }
    cur_tick = elapsed_sec * cur_anim->ticks_per_sec;

    for( int i=0; i<cur_anim->nr_tracks; i++ ) {
        const Animation::Track& track = cur_anim->tracks[i];
        int idx;
        float factor;
        vec3 v1, v2, vDt;
        Transform& nodeTf = skeleton[track.idx_bone_node].tf;
        if( track.nr_poss>1 ) {
            getKeyIdx(track.poss, track.nr_poss, cur_tick, idx, factor);
            v1 = track.poss[idx-1].value;
            v2 = track.poss[idx].value;
            vDt = v2 - v1;
            nodeTf.pos = v1 + vDt * factor;
        } else {
            nodeTf.pos = track.poss[0].value;
        }
        if( track.nr_scales>1 ) {
            getKeyIdx(track.scales, track.nr_scales, cur_tick, idx, factor);
            v1 = track.scales[idx-1].value;
            v2 = track.scales[idx].value;
            vDt = v2 - v1;
            nodeTf.scale = v1 + vDt * factor;
        } else {
            nodeTf.scale = track.scales[0].value;
        }
        if( track.nr_oris>1 ) {
            getKeyIdx(track.oris, track.nr_oris, cur_tick, idx, factor);
            quat o1 = track.oris[idx-1].value;
            quat o2 = track.oris[idx].value;
            nodeTf.ori = slerp(o1,o2,factor);
        } else {
            nodeTf.ori = track.oris[0].value;
        }
        
        nodeTf.update();
    }

    updateMtxBones();
}