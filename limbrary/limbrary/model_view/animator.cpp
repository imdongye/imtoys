#include <limbrary/model_view/animator.h>
#include <limbrary/asset_lib.h>
#include <limbrary/application.h>
#include <limbrary/model_view/model.h>
#include <algorithm>

using namespace lim;
using namespace glm;

void BoneNode::treversal(std::function<void(BoneNode& node, const glm::mat4& transform)> callback, const glm::mat4& prevTransform ) {
	const glm::mat4 curTf = prevTransform * tf.mtx;
	callback(*this, curTf);

	for( BoneNode& child : childs ) {
		child.treversal(callback, curTf);
	}
}
void BoneNode::clear() {
    bone_idx = -1;
	childs.clear();
}

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
    bone_root.clear();
    bone_tfs.clear();
    mtx_Bones.clear();
    cur_anim = nullptr;
    state = State::STOP;
}
static Transform* getDstBoneTf(BoneNode& dst, const BoneNode& src, const Transform* srcTf) {
    if( &src.tf == srcTf ) {
        return &dst.tf;
    }
    for(int i=0; i<src.childs.size(); i++) {
        Transform* ptr = getDstBoneTf(dst.childs[i], src.childs[i], srcTf);
        if(ptr)
            return ptr;
    }
    return nullptr;
}
static void copyBoneTfs(Animator& dst, const Animator& src) {
    dst.bone_tfs.clear();
    dst.bone_tfs.reserve(src.bone_tfs.size());
    for(const Transform* pSrcTf: src.bone_tfs) {
        dst.bone_tfs.push_back(getDstBoneTf(dst.bone_root, src.bone_root, pSrcTf));
    }
}
Animator& Animator::operator=(const Animator& src) {
    bone_root = src.bone_root;
    // bone_tfs = src.bone_tfs;
    copyBoneTfs(*this, src);
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
    prev_key_idxs.clear();
    prev_key_idxs.resize(cur_anim->tracks.size(), Animation::TrackIdx());
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
    const int prevIdx = idx;
    int i;
    for( i=prevIdx; i<nrKeys; i++ ) {
        if( curTick < keys[i].tick ) {
            idx = i;
            float dt = keys[i].tick - keys[i-1].tick;
            float diff = curTick - keys[i-1].tick;
            factor = diff / dt;
            return;
        }
    }
}

void Animator::updateMtxBones() {
    bone_root.treversal([&](const BoneNode& node, const glm::mat4& transform) {
        int boneIdx = node.bone_idx;
        if( boneIdx<0 )
            return;
        mtx_Bones[boneIdx] = transform * md_data->bone_offsets[boneIdx];
    });
}
void Animator::update(float dt) {
    if( state!=State::PLAY || !cur_anim )
        return;
    elapsed_sec += dt;
    if( elapsed_sec > duration_sec ) {
        if( is_loop ) {
            elapsed_sec = fmod(elapsed_sec, duration_sec);
        } else {
            state = State::STOP;
            return;
        }
        std::fill(prev_key_idxs.begin(), prev_key_idxs.end(), Animation::TrackIdx());
    }
    cur_tick = elapsed_sec * cur_anim->ticks_per_sec;

    for( int i=0; i<cur_anim->nr_tracks; i++ ) {
        const Animation::Track& track = cur_anim->tracks[i];
        Animation::TrackIdx& prevKeyIdx = prev_key_idxs[i];
        int idx;
        float factor;
        vec3 v1, v2, vDt;
        Transform& nodeTf = *bone_tfs[track.bone_tf_idx];
        if( track.nr_poss>1 ) {
            idx = prevKeyIdx.pos;
            getKeyIdx(track.poss, track.nr_poss, cur_tick, idx, factor);
            prevKeyIdx.pos = idx;

            v1 = track.poss[idx-1].value;
            v2 = track.poss[idx].value;
            vDt = v2 - v1;
            nodeTf.pos = v1 + vDt * factor;
        } else {
            nodeTf.pos = track.poss[0].value;
        }
        if( track.nr_scales>1 ) {
            idx = prevKeyIdx.scale;
            getKeyIdx(track.scales, track.nr_scales, cur_tick, idx, factor);
            prevKeyIdx.scale = idx;

            v1 = track.scales[idx-1].value;
            v2 = track.scales[idx].value;
            vDt = v2 - v1;
            nodeTf.scale = v1 + vDt * factor;
        } else {
            nodeTf.scale = track.scales[0].value;
        }
        if( track.nr_oris>1 ) {
            idx = prevKeyIdx.ori;
            getKeyIdx(track.oris, track.nr_oris, cur_tick, idx, factor);
            prevKeyIdx.ori = idx;

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