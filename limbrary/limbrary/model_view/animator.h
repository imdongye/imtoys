/*

2024-06-14 / im dong ye
edit learnopengl code

Note:
nr_bones(mesh) != nr_tracks != nr_bone_nodes
animations를 model(data)에서 공유하면 Track에 BoneNode를 


animation key 선택 O(N)을 줄이기 위한 방법
1. 이전 index값을 가지고 사용. O(1)
    Animation은 View마다 공유되기때문에 캐릭터마다 저장해야한다.
    캐릭터수 * 블랜딩할 애니메이션수 * 애니메이션의 트랙개수 의 크기로 가변길이로 저장된다. 
    캐릭터가 10개라고하면 118kb정도가 들고 가변길이라 관리에도 불편하다.
    그리고 액션게임에서 타임라인점프가 많은경우 이득을 보기 어렵다.
2. tick을 키로 BST로 저장해서 엣지에 해당하는 키두개를 사용. O(logN)

        

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
            // tick, value
            std::map<float, glm::vec3> poss;
            std::map<float, glm::vec3> scales;
            std::map<float, glm::quat> oris;
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
        enum State : int
        {
            ST_PLAY,
            ST_PAUSE,
            ST_STOP,
        };
        State state = ST_STOP;
        bool is_enabled = false;
        int nr_bone_nodes; // skeletion.size()
        std::vector<BoneNode> skeleton;
        std::vector<glm::mat4> mtx_Bones; // nr_bones
        const Animation* cur_anim = nullptr;
        const Model* md_data = nullptr;
        double elapsed_sec, duration_sec;
        float cur_tick = 0.f;
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