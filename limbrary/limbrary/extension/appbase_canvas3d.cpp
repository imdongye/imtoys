
#include "appbase_canvas3d.h"
#include <limbrary/tools/limgui.h>
#include <imgui.h>

using namespace lim;
using namespace glm;
using namespace std;




AppBaseCanvas3d::AppBaseCanvas3d(int winWidth, int winHeight, const char* title, bool vsync
    , int nrMaxQuads, int nrMaxSpheres, int nrMaxCylinders) 
    : AppBase(winWidth, winHeight, title, vsync)
    , vp("canvas3d", new FramebufferMs())
    , light()
    , max_nr_quads(nrMaxQuads)
    , max_nr_spheres(nrMaxSpheres)
    , max_nr_cylinders(nrMaxCylinders)
    , ms_quad(1.f, 1.f, true, false)
    , ms_sphere(1.f, 1, true, false)
    , ms_cylinder(1.f, 1.f, 8, true, false)
{
    ms_quad.initGL(false);
    ms_sphere.initGL(false);
    ms_cylinder.initGL(false);

    quads.resize(max_nr_quads);
    spheres.resize(max_nr_spheres);
    cylinders.resize(max_nr_cylinders);

    GLsizeiptr bufSize;

    bufSize = max_nr_quads*sizeof(PrimInfo);
    glGenBuffers(1, &buf_quads);
    glBindBuffer(GL_ARRAY_BUFFER, buf_quads);
    glBufferData(GL_ARRAY_BUFFER, bufSize, nullptr, GL_DYNAMIC_DRAW);

    bufSize = max_nr_spheres*sizeof(PrimInfo);
    glGenBuffers(1, &buf_spheres);
    glBindBuffer(GL_ARRAY_BUFFER, buf_spheres);
    glBufferData(GL_ARRAY_BUFFER, bufSize, nullptr, GL_DYNAMIC_DRAW);

    bufSize = max_nr_cylinders*sizeof(PrimInfo);
    glGenBuffers(1, &buf_cylinders);
    glBindBuffer(GL_ARRAY_BUFFER, buf_cylinders);
    glBufferData(GL_ARRAY_BUFFER, bufSize, nullptr, GL_DYNAMIC_DRAW);

    prog.name = "canvas3d_render";
    prog.attatch("canvas3d.vs").attatch("canvas3d.fs").link();
    prog.name = "canvas3d_shadow";
    prog_shadow.attatch("canvas3d.vs").attatch("depth.fs").link();

    vp.camera.pivot = vec3(0, 1.0, 0);
    vp.camera.pos = vec3(0, 1.5, 3.4);
	vp.camera.updateViewMat();
    light.setShadowEnabled(true);
    LimGui::LightDirectionalEditorReset("d_light editor##canvas3d", "d_light shadow map##canvas3d");
}
AppBaseCanvas3d::~AppBaseCanvas3d()
{
    glDeleteBuffers(1, &buf_quads);
    glDeleteBuffers(1, &buf_spheres);
    glDeleteBuffers(1, &buf_cylinders);
}

void AppBaseCanvas3d::resetInstance()
{
    nr_quads=0;
    nr_spheres=0;
    nr_cylinders=0;
}
void AppBaseCanvas3d::updateInstance() const
{
    GLbitfield accessFlags = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    PrimInfo* pMapped;
    size_t bufSize;

    if( nr_quads>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_quads);
        bufSize = sizeof(PrimInfo)*nr_quads;
        pMapped = (PrimInfo*)glMapBufferRange(GL_ARRAY_BUFFER, 0, bufSize, accessFlags);
        memcpy(pMapped, quads.data(), bufSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    if( nr_spheres>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_spheres);
        bufSize = sizeof(PrimInfo)*nr_spheres;
        pMapped = (PrimInfo*)glMapBufferRange(GL_ARRAY_BUFFER, 0, bufSize, accessFlags);
        memcpy(pMapped, spheres.data(), bufSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    if( nr_cylinders>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_cylinders);
        bufSize = sizeof(PrimInfo)*nr_cylinders;
        pMapped = (PrimInfo*)glMapBufferRange(GL_ARRAY_BUFFER, 0, bufSize, accessFlags);
        memcpy(pMapped, cylinders.data(), bufSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
}
void AppBaseCanvas3d::drawInstance() const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_quads);
    glBindVertexArray(ms_quad.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_quad.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_quad.tris.size()*3, GL_UNSIGNED_INT, nullptr, nr_quads);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_spheres);
    glBindVertexArray(ms_sphere.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_sphere.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_sphere.tris.size()*3, GL_UNSIGNED_INT, nullptr, nr_spheres);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buf_cylinders);
    glBindVertexArray(ms_cylinder.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_cylinder.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_cylinder.tris.size()*3, GL_UNSIGNED_INT, nullptr, nr_cylinders);
}

void AppBaseCanvas3d::update()
{
    glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvasUpdate();
    resetInstance();
    canvasDraw();
    updateInstance();
    

    if( light.shadow ) {
        light.bakeShadowMap([&](const glm::mat4& mtx_View, const glm::mat4& mtx_Proj) {
            customDrawShadow(mtx_View, mtx_Proj);
            prog_shadow.use();
            prog_shadow.setUniform("mtx_View", mtx_View);
            prog_shadow.setUniform("mtx_Proj", mtx_Proj);
            drawInstance();
        });
    }

	vp.getFb().bind();
    
    customDraw(vp.camera, light);

    prog.use();
	vp.camera.setUniformTo(prog);
	light.setUniformTo(prog);

    drawInstance();


	vp.getFb().unbind();
}
void AppBaseCanvas3d::updateImGui() {
    ImGui::DockSpaceOverViewport();
    vp.drawImGui();
    LimGui::LightDirectionalEditor(light);
    canvasImGui();
}



void AppBaseCanvas3d::drawQuad(const vec3& p, const vec3& n, const vec2& sz, const vec3& color, const float alpha) const {
    vec3 s = {sz.x, sz.y, 1.f};
    vec3 axis = cross(vec3{0,0,1}, n);
    float l = length(axis);
    float angle = atan2f(l, n.z);
    mat4 mtx_Model;
    if( l>0.0000001 )
        mtx_Model = translate(p) * rotate(angle, axis) * scale(s);
    else 
        mtx_Model = translate(p) * scale(s);
    
    quads[nr_quads] = {mtx_Model, vec4(color, alpha)};
    nr_quads++;
}


void AppBaseCanvas3d::drawSphere(const vec3& p, const float w, const vec3& color, const float alpha) const {
    mat4 mtx_Model = translate(p) * scale(vec3(w));

    spheres[nr_spheres] = {mtx_Model, vec4(color, alpha)};
    nr_spheres++;
}


void AppBaseCanvas3d::drawCylinder( const vec3& p1, const vec3& p2, const float w, const vec3& color, const float alpha ) const {
    vec3 diff = p1 - p2;
    vec3 mid = (p1 + p2) * 0.5f;
    vec3 s = {w, length(diff), w};
    vec3 axis = cross(vec3{0,1,0}, diff);
    float l = length(axis);
    float angle = atan2f(l, diff.y);
    mat4 mtx_Model;
    if( l>0.0000001f )
        mtx_Model = translate(mid) * rotate(angle, axis) * scale(s);
    else 
        mtx_Model = translate(mid) * scale(s);
    
    cylinders[nr_cylinders] = {mtx_Model, vec4(color, alpha)};
    nr_cylinders++;
}


void AppBaseCanvas3d::drawQuad(const mat4& mtx, const vec3& color, const float alpha) const {
    quads[nr_quads] = {mtx, vec4(color, alpha)};
    nr_quads++;
}
void AppBaseCanvas3d::drawSphere(const mat4& mtx, const vec3& color, const float alpha) const {
    spheres[nr_spheres] = {mtx, vec4(color, alpha)};
    nr_spheres++;
}
void AppBaseCanvas3d::drawCylinder( const mat4& mtx, const vec3& color, const float alpha ) const {
    cylinders[nr_cylinders] = {mtx, vec4(color, alpha)};
    nr_cylinders++;
}