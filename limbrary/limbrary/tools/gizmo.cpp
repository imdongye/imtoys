/*
Todo:
    최적화: 변환행렬케싱후
    마지막에 한번에 인스턴스드로잉
*/

#include <limbrary/tools/gizmo.h>
#include <limbrary/model_view/mesh_maked.h>
#include <glm/gtx/transform.hpp>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;

namespace
{
    Mesh* ms_sphere;
    Mesh* ms_cone;
    Mesh* ms_cylinder;
    Program* prog_gizmo;
}

void gizmo::init()
{
    ms_sphere = new MeshIcoSphere(1.f, 0, true, false);
    ms_sphere->initGL();
    ms_cone = new MeshCone(1.f, 1.f, 8, true, false);
    ms_cone->initGL();
    ms_cylinder = new MeshCylinder(1.f, 1.f, 6, true, false);
    ms_cylinder->initGL();

    prog_gizmo = new Program("gizmo");
    prog_gizmo->attatch("gizmo.vs").attatch("gizmo.fs").link();
}

void gizmo::deinit()
{
    delete ms_sphere;
    delete ms_cone;
    delete ms_cylinder;
    delete prog_gizmo;
}

void gizmo::drawPoint(const vec3& wPos, const vec4& col, float scale, const Camera& cam, bool screenScaling)
{
    float scaleFactor = scale;
    if(screenScaling) {
        scaleFactor *= glm::distance(wPos, cam.pos) * cam.fovy * 0.005f;
    }
    mat4 mtx_Model = glm::translate(wPos) * glm::scale(vec3(scaleFactor));

    prog_gizmo->use();
    cam.setUniformTo(*prog_gizmo);
    prog_gizmo->setUniform("color", col);
    prog_gizmo->setUniform("mtx_Model", mtx_Model);
    ms_sphere->bindAndDrawGL();
}

void gizmo::drawArrow(const vec3& wPos, const vec3 wDir, const vec4& col, float length, float width, const Camera& cam, bool screenScaling)
{
    float clWidth = width*0.5f;
    float scaleFactor = 1.f;
    if(screenScaling) {
        scaleFactor *= glm::distance(wPos, cam.pos) * cam.fovy * 0.005f;
    }
    vec3 end = wPos + wDir * length * scaleFactor;
    vec3 mid = wPos + wDir * length * scaleFactor*0.5f;
    vec3 diff = end - wPos;
    vec3 s = {scaleFactor*clWidth, glm::length(diff), scaleFactor*clWidth};
    vec3 axis = cross(vec3{0,1,0}, diff); // rotate 내부에서 normalize함
    float l = glm::length(axis);
    float angle = atan2f(l, diff.y);

    mat4 mtx_Model;
    mat4 rotMtx(1);
    if( l>0.0000001f ) {
        rotMtx = glm::rotate(angle, axis);
        mtx_Model = glm::translate(mid) * rotMtx * glm::scale(s);
    }
    else 
        mtx_Model = glm::translate(mid) * glm::scale(s);
        
    prog_gizmo->use();
    cam.setUniformTo(*prog_gizmo);
    prog_gizmo->setUniform("color", col);
    prog_gizmo->setUniform("mtx_Model", mtx_Model);
    ms_cylinder->bindAndDrawGL();

    mat4 scaleMtx = glm::scale(vec3(scaleFactor*width));
    mtx_Model = glm::translate(wPos) * scaleMtx;
    prog_gizmo->setUniform("mtx_Model", mtx_Model);
    ms_sphere->bindAndDrawGL();

    mtx_Model = glm::translate(end) * rotMtx * scaleMtx;
    prog_gizmo->setUniform("mtx_Model", mtx_Model);
    ms_cone->bindAndDrawGL();
}