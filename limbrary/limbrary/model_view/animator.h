#ifndef __animator_h_
#define __animator_h_
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limbrary/program.h>
#include <vector>
#include <string>

namespace lim
{
    struct RdNode;
    class Model;

    struct Animation 
    {
    public:
        struct KeyVec3 {
            float tick;
            glm::vec3 value;
        };
        struct KeyQuat {
            float tick;
            glm::quat value;
        };
        struct BoneTrack
        {
            int nr_poss, nr_scales, nr_oris;
            std::vector<KeyVec3> poss;
            std::vector<KeyVec3> scales;
            std::vector<KeyQuat> oris;
            RdNode* target;
            int bone_idx;
        };
        std::string name;
        std::vector<BoneTrack> bone_tracks;
        double nr_ticks;
        int ticks_per_sec;
    };

    class Animator
    {
    public:
        enum class State {
            PLAY,
            PAUSE,
            STOP,
        };
        State state = State::STOP;
        Model* model;
        int anim_idx = -1;
        double start_sec, elapsed_sec, duration_sec;
        bool is_loop = false;
        std::vector<glm::mat4> mtx_Bones;
        std::vector<Animation> animations;
        
    public:
        Animator(const Model& src)		  = delete;
		Animator& operator=(const Model&) = delete;
		Animator(Model&&)			      = delete;
		Animator& operator=(Model&&)      = delete;

        Animator(Model* md);
        ~Animator();
        void play(int animIdx = -1);
        void pause();
        void stop();
        void setUniformTo(const Program& prog) const;
        void updateDefaultMtxBones();

    private:
        friend class Model;
        void updateMtxBonesSize();
        void update(float dt);
    };

}

#endif