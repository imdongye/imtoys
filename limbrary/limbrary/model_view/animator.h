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
    class Model;

    struct BoneNode {
        std::string name = "nonamed node";
        int bone_idx = -1;
        Transform transform;
        std::vector<BoneNode> childs;
        
        void treversal(std::function<bool(BoneNode& node, const glm::mat4& transform)> callback, const glm::mat4& prevTransform = glm::mat4(1));
        void clear();
    };

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
            std::string name;
            BoneNode* target;
            int nr_poss, nr_scales, nr_oris;
            std::vector<KeyVec3> poss;
            std::vector<KeyVec3> scales;
            std::vector<KeyQuat> oris;
        };
        std::string name;
        std::vector<BoneTrack> bone_tracks;
        double nr_ticks;
        int ticks_per_sec;
    };

    class Animator
    {
    public:
        BoneNode bone_root;

        enum class State {
            PLAY,
            PAUSE,
            STOP,
        };
        State state = State::STOP;

        int anim_idx = -1;
        double start_sec, elapsed_sec, duration_sec;
        bool is_loop = false;

		int nr_bones = 0;
		std::map<std::string, int> name_to_idx;
		std::vector<glm::mat4> offsets;
        std::vector<glm::mat4> mtx_Bones;
        std::vector<Animation> animations;
        
    public:
        Animator(const Model& src)		  = delete;
		Animator& operator=(const Model&) = delete;
		Animator(Model&&)			      = delete;
		Animator& operator=(Model&&)      = delete;

        Animator();
        ~Animator();
        void clear();

        void play(int animIdx = -1);
        void pause();
        void stop();
        void setUniformTo(const Program& prog) const;

    private:
        friend class Model;
        void updateMtxBonesSize();
        void update(float dt);
    };

}

#endif