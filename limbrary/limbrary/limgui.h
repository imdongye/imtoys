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

namespace LimGui
{
    IMGUI_API bool CheckBox3(const char* label, bool v[3]);
    IMGUI_API void Mat4(glm::mat4& m);
    IMGUI_API void Vec3(glm::vec3& v);
    
	// only one in updateImGui
    void ModelEditor(lim::ModelView& md);
    void ModelEditorReset();
}


#endif