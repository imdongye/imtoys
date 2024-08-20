#include <limbrary/model_view/animator.h>
#include <limbrary/tools/asset_lib.h>
#include <limbrary/tools/log.h>
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


static vec3 getInterpolatedValue( const std::vector<Animation::KeyVec3>& keys, float curTick ) {
    const int nrKeys = keys.size(); 
    Animation::KeyVec3 k1 = keys[0];
    Animation::KeyVec3 k2 = keys[1];
    for( int i=0; i<nrKeys; i++ ) {
        if( curTick < keys[i].tick ) {
            if(i==0) {
                k1 = keys[nrKeys-1];
                k1.tick = 0.f;
            } else {
                k1 = keys[i-1];
            }
            k2 = keys[i];
            break;
        }
    }
    float factor = (curTick - k1.tick) / (k2.tick - k1.tick);
    return k1.value + (k2.value - k1.value) * factor;
}

static quat getInterpolatedValue( const std::vector<Animation::KeyQuat>& keys, float curTick ) {
    const int nrKeys = keys.size();
    Animation::KeyQuat k1 = keys[0];
    Animation::KeyQuat k2 = keys[1];
    for( int i=0; i<nrKeys; i++ ) {
        if( curTick < keys[i].tick ) {
            if(i==0) {
                k1 = keys[nrKeys-1];
                k1.tick = 0.f;
            } else {
                k1 = keys[i-1];
            }
            k2 = keys[i];
            break;
        }
    }
    float factor = (curTick - k1.tick) / (k2.tick - k1.tick);
    return slerp(k1.value, k2.value, factor);
}


/* stl map version */
// Todo : handle iterator when curTick is lower then first tick
// static vec3 getInterpolatedValue( const std::map<float, vec3>& keys, float curTick ) {
//     auto it = keys.lower_bound( curTick );
//     float t2 = it->first;
//     vec3 v2 = it->second;
//     it--;
//     float t1 = it->first;
//     vec3 v1 = it->second;
//     float dt = t2 - t1;
//     float factor = (curTick - t1) / dt;
//     return v1 + (v2 - v1) * factor;
// }

// static quat getInterpolatedValue( const std::map<float, quat>& keys, float curTick ) {
//     auto it = keys.lower_bound( curTick );
//     float t2 = it->first;
//     quat v2 = it->second;
//     it--;
//     float t1 = it->first;
//     quat v1 = it->second;
//     float dt = t2 - t1;
//     float factor = (curTick - t1) / dt;
//     return slerp(v1, v2, factor);
// }


void Animator::updateMtxBones() {
    BoneNode& rootBoneNode = skeleton[0];
    rootBoneNode.tf_model_space = rootBoneNode.tf.mtx;

    if( rootBoneNode.idx_bone>=0) {
        assert(rootBoneNode.idx_bone==0);
        mtx_Bones[0] = rootBoneNode.tf_model_space * md_data->bone_offsets[0];
    }

    // recursive term
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
        nodeTf.pos = (track.nr_poss>1)    ?getInterpolatedValue(track.poss, cur_tick):track.poss[0].value;
        nodeTf.scale = (track.nr_scales>1)?getInterpolatedValue(track.scales, cur_tick):track.scales[0].value;
        nodeTf.ori = (track.nr_oris>1)    ?getInterpolatedValue(track.oris, cur_tick):track.oris[0].value;
        nodeTf.update();
    }

    updateMtxBones();
}