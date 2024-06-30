#include <limbrary/model_view/animator.h>
#include <limbrary/asset_lib.h>
#include <limbrary/application.h>
#include <limbrary/model_view/model.h>

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
    cur_anim = nullptr;
    state = State::STOP;
    bone_root.clear();
    bone_tfs.clear();
    mtx_Bones.clear();
}
Animator& Animator::operator=(const Animator& src) {
    bone_root = src.bone_root;
    cur_anim = src.cur_anim;
    init(src.md_data);
    start_sec = src.start_sec;
    elapsed_sec = src.elapsed_sec;
    duration_sec = src.duration_sec;
    is_loop = src.is_loop;
    mtx_Bones = src.mtx_Bones;
    return *this;
}

void Animator::setAnim(const Animation* anim) {
    cur_anim = anim;
    duration_sec = cur_anim->nr_ticks/cur_anim->ticks_per_sec;
    state = State::STOP;
}
void Animator::play() {
    if( !cur_anim ) {
        log::err("no selected animation\n");
        assert(false);
    }
    if( state == State::PAUSE )
        start_sec = glfwGetTime() - elapsed_sec;
    else {
        start_sec = glfwGetTime();
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
    // if( !cur_anim ) {
    if( state == State::STOP ) {
        prog.setUniform("is_Animated", false);
    } else {
        prog.setUniform("is_Animated", true);
        prog.setUniform("mtx_Bones", mtx_Bones);
    }
    
}


static void getKeyIdx(const std::vector<Animation::KeyVec3>& keys, double cur_tick, int& idx, float& factor ) {
    for( int i=0; i<keys.size(); i++ ) {
        if( cur_tick < keys[i].tick ) {
            idx = i;
            float dt = keys[i].tick - keys[i-1].tick;
            float diff = cur_tick - keys[i-1].tick;
            factor = diff / dt;
            return;
        }
    }
}
static void getKeyIdx(const std::vector<Animation::KeyQuat>& keys, double cur_tick, int& idx, float& factor ) {
    for( int i=0; i<keys.size(); i++ ) {
        if( cur_tick < keys[i].tick ) {
            idx = i;
            float dt = keys[i].tick - keys[i-1].tick;
            float diff = cur_tick - keys[i-1].tick;
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
    elapsed_sec = glfwGetTime() - start_sec;
    if( elapsed_sec > duration_sec ) {
        if( is_loop ) {
            elapsed_sec = elapsed_sec - duration_sec;
            start_sec += duration_sec;
        } else {
            state = State::STOP;
            return;
        }
    }
    cur_tick = elapsed_sec * cur_anim->ticks_per_sec;
    // cur_tick = fmod(cur_tick, cur_anim->nr_ticks);

    for( const Animation::Track& track : cur_anim->tracks ) {
        int idx;
        float factor;
        vec3 v1, v2, vDt;
        Transform& nodeTf = *bone_tfs[track.bone_tf_idx];
        if( track.nr_poss>1 ) {
            getKeyIdx(track.poss, cur_tick, idx, factor);
            v1 = track.poss[idx-1].value;
            v2 = track.poss[idx].value;
            vDt = v2 - v1;
            nodeTf.pos = v1 + vDt * factor;
        } else {
            nodeTf.pos = track.poss[0].value;
        }
        if( track.nr_scales>1 ) {
            getKeyIdx(track.scales, cur_tick, idx, factor);
            v1 = track.scales[idx-1].value;
            v2 = track.scales[idx].value;
            vDt = v2 - v1;
            nodeTf.scale = v1 + vDt * factor;
        } else {
            nodeTf.scale = track.scales[0].value;
        }
        if( track.nr_oris>1 ) {
            getKeyIdx(track.oris, cur_tick, idx, factor);
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