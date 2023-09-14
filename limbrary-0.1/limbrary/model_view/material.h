/*

2023-09-07 / im dongye


*/

#ifndef __material_h_
#define __material_h_

#include <glm/glm.hpp>
#include "../texture.h"
#include "../program.h"

namespace lim
{
    class Material
    {
    public:
        glm::vec4 Kd = {0.27f, 0.79f, 0.69f, 0.f};  // {Kd, alpha}
        glm::vec4 Ks = {1.f, 1.f, 1.f, 100.f};      // {Ks, shininess[0, 128]}
        glm::vec3 Ka = {0.2f, 0.2f, 0.2f};          // ambient
        glm::vec3 Ke = {0.f, 0.f, 0.f};             // emission
        glm::vec3 Tf = {0.f, 0.f, 0.f};             // transmission
        float Ni     = 1.45f;                       // index of refraction
        
        Texture* map_Kd = nullptr;
        Texture* map_Ks = nullptr;
        Texture* map_Ka = nullptr;
        Texture* map_Ns = nullptr;
        Texture* map_Bump = nullptr;
        bool bumpIsNormal = true;

        float bump_height = 100;
        float tex_delta = 0.00001f;

        Program* prog = nullptr;
    };
}
#endif