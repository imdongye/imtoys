#include <limbrary/model_view/animator.h>
#include <limbrary/asset_lib.h>
#include <limbrary/application.h>
#include <limbrary/model_view/model.h>

using namespace lim;
using namespace glm;

void BoneNode::treversal(std::function<bool(BoneNode& node, const glm::mat4& transform)> callback, const glm::mat4& prevTransform )
{
	const glm::mat4 curTf = prevTransform*transform.mtx;

	if( !callback(*this, curTf) )
		return;

	for( BoneNode& child : childs ) {
		child.treversal(callback, curTf);
	}
}
void BoneNode::clear() {
	childs.clear();
}


Animator::Animator() {
    AssetLib::get().app->update_hooks[this] = [this](float dt) {
        update(dt);
    };
}
Animator::~Animator() {
    AssetLib::get().app->update_hooks.erase(this);
    clear();
}
void Animator::clear() {
    nr_bones = 0;
    bone_root.clear();
    name_to_idx.clear();
	offsets.clear();
    mtx_Bones.clear();
    animations.clear();
}

void Animator::play(int animIdx) {
    if(animIdx<0 && anim_idx<0) {
        log::err("no selected animation\n");
        assert(false);
    }
    if( animIdx!=anim_idx ) {
        anim_idx = animIdx;
        duration_sec = animations[anim_idx].nr_ticks/animations[anim_idx].ticks_per_sec;
        state = State::STOP;
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
void Animator::update(float dt) {
    if(state!=State::PLAY || anim_idx<0 )
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
    Animation& curAnim = animations[anim_idx];
    double cur_tick = elapsed_sec * curAnim.ticks_per_sec;
    cur_tick = fmod(cur_tick, curAnim.nr_ticks);

    for( const Animation::BoneTrack& track : curAnim.bone_tracks ) {
        int idx;
        float factor;
        vec3 v1, v2, vDt;
        Transform& nodeTf = track.target->transform;
        if( track.nr_poss>1 ) {
            getKeyIdx(track.poss, cur_tick, idx, factor);
            v1 = track.poss[idx-1].value;
            v2 = track.poss[idx].value;
            vDt = v2 - v1;
            nodeTf.pos = v1 + dt * factor;
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

    bone_root.treversal([&](const BoneNode& node, const glm::mat4& transform) {
        int boneIdx = node.bone_idx;
        if( boneIdx<0 )
            return true;
        mat4 offset = offsets[boneIdx];
        mtx_Bones[boneIdx] = transform*offset;
        return true;
    });
}