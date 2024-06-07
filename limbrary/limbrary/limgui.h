#include <imgui.h>
#include <glm/glm.hpp>

namespace LimGui
{
    IMGUI_API bool CheckBox3(const char* label, bool v[3]);
    IMGUI_API void Mat4(glm::mat4& m);
    IMGUI_API void Vec3(glm::vec3& v);
}