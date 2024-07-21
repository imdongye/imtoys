/*

2023-09-07 / im dongye

Note:
Program과 Texture은 material에 종속되지 않음. 생명주기 따로 관리해줘야함.

Ambient Roughness Metalness (MF_ARM) => map_Roughness
Shininess (MF_SHININESS)             => map_Roughness
Shininess (MF_SHININESS)             => map_Roughness


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
        enum FactorFlag {
            FF_NONE         = 0,
            FF_COLOR_BASE   = 1<<0,
            FF_SPECULAR     = 1<<1,
            FF_AMBIENT      = 1<<2,
            FF_EMISSION     = 1<<3,
            FF_TRANSMISSION = 1<<4,
            FF_REFRACITI    = 1<<5,
            FF_OPACITY      = 1<<6,
            FF_SHININESS    = 1<<7,
            FF_ROUGHNESS    = 1<<8,
            FF_METALNESS    = 1<<9,
        };
        enum MapFlag{
            MF_NONE       = 0,
            MF_COLOR_BASE = 1<<0, // 1
            MF_SPECULAR   = 1<<1, // 2
            MF_HEIGHT     = 1<<2, // 4
            MF_NOR        = 1<<3, // 8
            MF_AMB_OCC    = 1<<4, // 16
            MF_ROUGHNESS  = 1<<5, // 32
            MF_METALNESS  = 1<<6, // 64
            MF_EMISSION   = 1<<7, // 128
            MF_OPACITY    = 1<<8, // 256
            MF_MR         = 1<<9, // 518
            MF_ARM        = 1<<10,// 1026
            MF_SHININESS  = 1<<11,// 2052
        };
    
        std::string name = "nonamed mat";

        int factor_flags = 0; // just for export

        glm::vec3 BaseColor = {0.27f, 0.79f, 0.69f}; // 지구의 모든 물체는 반사율 0이 될수없음.
        glm::vec3 SpecColor = glm::vec3(1.f);
        glm::vec3 AmbientColor = glm::vec3(0.003f);
        glm::vec3 EmissionColor = glm::vec3(0.f);
        glm::vec3 F0 = glm::vec3(0.21f); // plastic high assimp로드되지 않음. 
        float Transmission = 0.f;
        float Refraciti = 0.f;           // ior
        float Opacity = 1.f;
        float Shininess = 100.f;         // assimp로드 기본값 낮아서 로드하지 않음.
        float Roughness = 0.3f;
        float Metalness = 0.0f;
        float BumpHeight = 100;
        float TexDelta = 0.00001f;

        int map_Flags = 0;
        Texture* map_ColorBase = nullptr;     // Diffuse Color
        Texture* map_Specular = nullptr;
        Texture* map_Bump = nullptr;          // MF_Height, MF_Nor 로 노멀맵인지 height맵인지 구분
        Texture* map_AmbOcc = nullptr;
        Texture* map_Roughness = nullptr;     // ARM, RM, shineness도 들어갈 수 있음.
        Texture* map_Metalness = nullptr;     
        Texture* map_Emission = nullptr;
        Texture* map_Opacity = nullptr;       


        const Program* prog; // AssetLib::get().prog_dnv
		std::function<void(const Program&)> set_prog = nullptr;


        Material();
        Material(const Material& mat) = default;
        void setUniformTo(const Program& prg) const;
    };
}
#endif