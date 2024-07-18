/*

2024-06-14 / im dong ye
edit learnopengl code

Note:
nr_bones(mesh) != nr_tracks != nr_bone_nodes
animations를 model(data)에서 공유하면 Track에 BoneNode를 


*/

#ifndef __animator_h_
#define __animator_h_
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limbrary/program.h>
#include <vector>
#include <string>
#include <limbrary/model_view/transform.h>
#include <map>

namespace lim
{
    struct Animation 
    {
        struct KeyVec3 {
            float tick;
            glm::vec3 value;
        };
        struct KeyQuat {
            float tick;
            glm::quat value;
        };
        struct Track
        {
            std::string name;
            int idx_bone_node = -1;
            int nr_poss, nr_scales, nr_oris;
            std::vector<KeyVec3> poss;
            std::vector<KeyVec3> scales;
            std::vector<KeyQuat> oris;
        };
        // for animator
        struct TrackIdx
        {
            int pos=0, scale=0, ori=0;
        };
        std::string name;
        int nr_tracks;
        std::vector<Track> tracks;
        double nr_ticks;
        int ticks_per_sec;
    };
    


    // assume nr_bone_nodes(skeleton) >= nr_bones
    struct BoneNode {
        std::string name = "nonamed node";
        Transform tf; // bone space
        glm::mat4 tf_model_space;
        int idx_bone = -1; // for mtx_Bones in ModelView, bone_offsets in Model
        int idx_parent_bone_node = -1; // for skeleton in ModelView
        int nr_childs;
    };

    class Model;

    class Animator
    {
    public:
        enum class State {
            PLAY,
            PAUSE,
            STOP,
        };
        State state = State::STOP;

        int nr_bone_nodes;
        std::vector<BoneNode> skeleton;
        std::vector<glm::mat4> mtx_Bones;
        const Animation* cur_anim = nullptr;
        std::vector<Animation::TrackIdx> prev_key_idxs;
        const Model* md_data = nullptr;
        double elapsed_sec, duration_sec;
        double cur_tick = 0.0;
        bool is_loop = false;

        
    public:
        Animator(const Animator& src)		 = delete;
		Animator(Animator&&)			     = delete;
		Animator& operator=(Animator&&)      = delete;

        Animator();
		Animator& operator=(const Animator& src); // todo default
        ~Animator();
        void init(const Model* md);
        void clear();
        
        void setAnim(const Animation* anim);
        void setTimeline(const float pct, bool updateMenualy=false);
        void play();
        void pause();
        void stop();
        void setUniformTo(const Program& prog) const;
        void updateMtxBones();

    private:
        void update(float dt);
    };

}

#endif