#include <limbrary/tools/gizmo.h>
#include <limbrary/model_view/mesh_maked.h>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

namespace
{
    const Mesh* ms_cone;
    const Mesh* ms_cilinder;
    const Program* prog_gizmo;
}

void gizmo::init()
{
    // ms_cone = new MeshCylinder
}

void gizmo::destroy()
{

}

void gizmo::drawArrow(const vec3& wPos, const vec3 wDir, const vec4& col, float sScale)
{

}