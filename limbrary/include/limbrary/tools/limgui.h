/*
	imdongye@naver.com
	fst: 2024-06-15
	lst: 2024-06-15

Note:
    imgui for limbrary
    
    Exception viewport draws as a member function
*/


#ifndef __limgui_h_
#define __limgui_h_


#include <imgui.h>
#include <glm/glm.hpp>
#include <limbrary/3d/scene.h>
#include <limbrary/3d/viewport_with_cam.h>

namespace LimGui
{
    IMGUI_API bool CheckBox3(const char* label, bool v[3]);
    IMGUI_API void Mat4(const glm::mat4& m);
    IMGUI_API void Vec3(const glm::vec3& v);
    
    void PlotVal(const char* name, const char* postFix, float value, int bufSize=90);
    void PlotVal(const char* name, const char* postFix, int bufSize=90);
    void PlotValAddValue(const char* name, float value, int bufSize=90);

    bool IsWindowHidden();
}

// only one in updateImGui
namespace LimGui
{
    void resetEditors();

    void ModelViewEditor(lim::ModelView& md);
    void ModelDataEditor(lim::Model& md);
    void RdNodeEditor(lim::RdNode& rd);
    void BoneNodeEditor(lim::BoneNode& rd);
    lim::RdNode*    getPickedRenderNode();
    lim::BoneNode*  getPickedBoneNode();

    void LightDirectionalEditor(lim::LightDirectional& lit);
    void LightSpotEditor(lim::LightSpot& lit);
    void LightOmniEditor(lim::LightOmni& lit);
    void IBLightEditor(lim::IBLight& lit);

    void SceneEditor(lim::Scene& scn, lim::ViewportWithCam& vp);
}

namespace lim
{
    inline ImVec2 toIg(const glm::vec2& v) { return {v.x, v.y}; }
    inline ImVec2 toIg(const glm::ivec2& v) { return {float(v.x), float(v.y)}; }
    inline glm::vec2 toGlm(const ImVec2& v) { return {v.x, v.y}; }
    inline ImTextureID texIdToIg(GLuint texId) { return (ImTextureID)(intptr_t)(texId); }
    inline bool isSame(const ImVec2& v1, const ImVec2& v2) { return v1.x == v2.x && v1.y == v2.y; }
}


#endif