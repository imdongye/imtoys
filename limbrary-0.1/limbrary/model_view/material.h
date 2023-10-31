/*

2023-09-07 / im dongye

Note:
Program과 Texture은 material에 종속되지 않음. 생명주기 따로 관리해줘야함.

From mtl format: https://paulbourke.net/dataformats/mtl/
                 https://people.sc.fsu.edu/~jburkardt/data/mtl/mtl.html
*/

#ifndef __material_h_
#define __material_h_

#include <glm/glm.hpp>
#include "../texture.h"
#include "../program.h"

namespace lim
{
    // copyable
    struct Material
    {
        enum MapFlags{
            MF_None  = 0,
            MF_Kd    = 1<<0, // 1
            MF_Ks    = 1<<1, // 2
            MF_Ka    = 1<<2, // 4
            MF_Ns    = 1<<3, // 8
            MF_Height= 1<<4, // 16
            MF_Nor   = 1<<5, // 32
        };

        glm::vec3 Kd = {0.27f, 0.79f, 0.69f}; // {Kd, alpha}
        glm::vec3 Ks = {1.f, 1.f, 1.f};       // {Ks, shininess[0, 128]}
        glm::vec3 Ka = {0.2f, 0.2f, 0.2f};    // ambient
        glm::vec3 Ke = {0.f, 0.f, 0.f};       // emission
        glm::vec3 Tf = {0.f, 0.f, 0.f};       // transmission
        float d         = 1.f;                // opacity non-transparency
        float Tr        = 0.f;                // transparency
        float Ns        = 100.f;              // shininess
        float Ni        = 1.45f;              // index of refraction
        float roughness = 0.3f;               // brdf param
        float metalness = 0.0f;

        int map_Flags = 0;
        Texture* map_Kd = nullptr;
        Texture* map_Ks = nullptr;
        Texture* map_Ka = nullptr;
        Texture* map_Ns = nullptr;            // shininess
        Texture* map_Bump = nullptr; // MF_Height, MF_Nor 로 노멀맵인지 height맵인지 구분
        Texture* map_ARM = nullptr;

        float bumpHeight = 100;
        float texDelta = 0.00001f;

        Program* prog = nullptr;
		std::function<void(const Program&)> set_prog;
    };
}
#endif