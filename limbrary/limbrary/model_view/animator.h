#ifndef __animator_h_
#define __animator_h_
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>
#include <map>

// namespace lim
// {
//     struct KeyPos {
//         float time_stamp;
//         glm::vec3 pos;
//     };
//     struct KeyScale {
//         float time_stamp;
//         glm::vec3 scale;
//     };
//     struct KeyOri {
//         float time_stamp;
//         glm::quat pos;
//     };

//     class Bone
//     {
//     private:
//         std::vector<KeyPos> poss;
//         std::vector<KeyPos> scales;
//         std::vector<KeyPos> oris;
//         glm::mat4 local_mtx;
//         std::string name;
//         int id;
//     public:
//         Bone(const std::string_view name, int ID, const )
//     };

//     struct BoneInfo
// 	{
// 		int id;
// 		glm::mat4 offset;
// 	};

//     class Animation 
//     {
//     private:
//         std::vector<Bone> bones;
//         int nr_bones = 0;
// 		std::map<std::string, BoneInfo> bone_map;

//         int tics_per_sec;

//         std::string name;
//         int id;
//     public:
//         Animation(const std::string_view name, int ID)
//     };

//     class Animator
//     {
//     private:
//         std::vector<glm::mat4> mtx_Bones;
//         Animation* current_animation = nullptr;
//         float cur_time;
//         float delta_time;
//     public:
//         Animator(const std::string_view name, int ID)
//     }

// }

#endif