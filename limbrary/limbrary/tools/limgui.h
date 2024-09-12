/*

2024-06-15 / im dong ye


*/


#ifndef __limgui_h_
#define __limgui_h_


#include <imgui.h>
#include <glm/glm.hpp>
#include <limbrary/model_view/scene.h>
#include <limbrary/model_view/camera_man.h>

namespace LimGui
{
    IMGUI_API bool CheckBox3(const char* label, bool v[3]);
    IMGUI_API void Mat4(const glm::mat4& m);
    IMGUI_API void Vec3(const glm::vec3& v);
    
    void PlotVal(const char* name, const char* postFix, float value, int bufSize=90);
    void PlotVal(const char* name, const char* postFix, int bufSize=90);
    void PlotValAddValue(const char* name, float value, int bufSize=90);
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

    void SceneEditor(lim::Scene& scn);

    
	void Viewport(lim::Viewport& vp);
    void ViewportWithGuizmo(lim::ViewportWithCamera& vp
        , std::function<void(lim::Viewport*)> guizmoHook = nullptr);
    void ViewportWithSceneGuizmo(lim::ViewportWithCamera& vp
        , std::function<void(lim::Viewport*)> guizmoHook = nullptr);
}

namespace lim
{
    inline ImVec2 toIg(const glm::vec2& v) { return {v.x, v.y}; }
    inline glm::vec2 toGlm(const ImVec2& v) { return {v.x, v.y}; }
}


#endif