/*

2024-06-15 / im dong ye


*/


#ifndef __limgui_h_
#define __limgui_h_


#include <imgui.h>
#include <glm/glm.hpp>
#include <limbrary/model_view/model.h>
#include <limbrary/framebuffer.h>
#include <limbrary/model_view/camera.h>
#include <limbrary/model_view/light.h>

namespace LimGui
{
    IMGUI_API bool CheckBox3(const char* label, bool v[3]);
    IMGUI_API void Mat4(const glm::mat4& m);
    IMGUI_API void Vec3(const glm::vec3& v);
    
	// only one in updateImGui
    void ModelEditor(lim::ModelView& md);
    void ModelEditorReset(const char* windowName = "Model Editor");
    lim::RdNode*    getPickedRenderNode();
    lim::BoneNode*  getPickedBoneNode();


    void LightDirectionalEditor(lim::LightDirectional& lit);
    void LightDirectionalEditorReset(const char* name="d_light editor", const char* smName="d_light shadow map");

    void PlotVal(const char* name, const char* postFix, float value, int bufSize=90);
    void PlotVal(const char* name, const char* postFix, int bufSize=90);
    void PlotValAddValue(const char* name, float value, int bufSize=90);
}
namespace lim
{
    inline ImVec2 toIG(const glm::vec2& v) { return {v.x, v.y}; }
    inline glm::vec2 toGLM(const ImVec2& v) { return {v.x, v.y}; }
}


#endif