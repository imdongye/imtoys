/*

2023-09-07 / im dongye


*/

#ifndef __material_h_
#define __material_h_

#include <glm/glm.hpp>
#include <limbrary/texture.h>

namespace lim
{
    class Material
    {
    public:
        glm::vec4 Kd = {0.27f, 0.79f, 0.69f, 0.f}; // {Kd, d}
        glm::vec4 Ks = {1.f, 1.f, 1.f, 100.f};     // {Ks, }
        glm::vec3 Ka = {0.2f, 0.2f, 0.2f};
        glm::vec3 Ke = {0.f, 0.f, 0.f};
        glm::vec3 Tf = {0.f, 0.f, 0.f};
        float Ns = 100.f; // shininess
        float Ni = 1.45;  // index of refraction
        
        Texture* map_Kd = nullptr;
        Texture* map_Ks = nullptr;
        Texture* map_Ka = nullptr;
        Texture* map_Ns = nullptr;
        Texture* map_bump = nullptr;
    };
}
#endif