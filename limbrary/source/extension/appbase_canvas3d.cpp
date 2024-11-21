#include <limbrary/extension/appbase_canvas3d.h>
#include <limbrary/tools/limgui.h>
#include <imgui.h>
#include <glm/gtx/transform.hpp>
#include <limbrary/using_in_cpp/glm.h>
#include <limbrary/using_in_cpp/std.h>
using namespace lim;

static constexpr size_t pi_size = sizeof(vec4)*5;

static void updatePrimBufSize(GLuint buf, GLsizeiptr capacity)
{
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, capacity*pi_size, nullptr, GL_DYNAMIC_DRAW);
}

static void updatePrimInfoToVAO(GLuint vao)
{
    constexpr size_t vec4Size = sizeof(glm::vec4);

    glBindVertexArray(vao);

    glEnableVertexAttribArray(3); // matrix
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, pi_size, (void*)0);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4); 
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, pi_size, (void*)(1*vec4Size));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5); 
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, pi_size, (void*)(2*vec4Size));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6); 
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, pi_size, (void*)(3*vec4Size));
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7); // color
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, pi_size, (void*)(4*vec4Size));
    glVertexAttribDivisor(7, 1);

    glBindVertexArray(0);
}

AppBaseCanvas3d::AppBaseCanvas3d(int winWidth, int winHeight, const char* title, bool vsync
    , int capacityQuads, int capacitySpheres, int capacityCylinders) 
    : AppBase(winWidth, winHeight, title, vsync)
    , vp(new FramebufferMs(), "Canvas3d")
    , light(true)
    , capacity_quads(capacityQuads)
    , capacity_spheres(capacitySpheres)
    , capacity_cylinders(capacityCylinders)
    , ms_quad(1.f, 1.f, true, false)
    , ms_sphere(1.f, 1, true, false)
    , ms_cylinder(1.f, 1.f, 8, true, false)
{
    ms_quad.initGL(false);
    ms_sphere.initGL(false);
    ms_cylinder.initGL(false);

    quads.reserve(capacity_quads);
    spheres.reserve(capacity_spheres);
    cylinders.reserve(capacity_cylinders);

     

    glGenBuffers(1, &buf_quads);
    updatePrimBufSize(buf_quads, capacity_quads);
    updatePrimInfoToVAO(ms_quad.vao);

    glGenBuffers(1, &buf_spheres);
    updatePrimBufSize(buf_spheres, capacity_spheres);
    updatePrimInfoToVAO(ms_sphere.vao);

    glGenBuffers(1, &buf_cylinders);
    updatePrimBufSize(buf_cylinders, capacity_cylinders);
    updatePrimInfoToVAO(ms_cylinder.vao);


    prog.name = "canvas3d_render";
    prog.attatch("canvas3d.vs").attatch("canvas3d.fs").link();
    prog.name = "canvas3d_shadow";
    prog_shadow.attatch("canvas3d.vs").attatch("depth.fs").link();

    vp.camera.pivot = vec3(0, 1.0, 0);
    vp.camera.pos = vec3(0, 1.5, 3.4);
	vp.camera.updateViewMtx();
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
    quads.clear();
    spheres.clear();
    cylinders.clear();
}
void AppBaseCanvas3d::updateInstance() const
{
    if( nr_quads>0 ) {
        if( nr_quads > capacity_quads ) {
            capacity_quads = glm::max(capacity_quads*2, nr_quads);
            updatePrimBufSize(buf_quads, capacity_quads);
        }
        glBindBuffer(GL_ARRAY_BUFFER, buf_quads);
        glBufferSubData(GL_ARRAY_BUFFER, 0, nr_quads*pi_size, quads.data());
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    if( nr_spheres>0 ) {
        if( nr_spheres > capacity_spheres ) {
            capacity_spheres = glm::max(capacity_spheres*2, nr_spheres);
            updatePrimBufSize(buf_spheres, nr_spheres);
        }
        glBindBuffer(GL_ARRAY_BUFFER, buf_spheres);
        glBufferSubData(GL_ARRAY_BUFFER, 0, nr_spheres*pi_size, spheres.data());
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }

    if( nr_cylinders>0 ) {
        if( nr_cylinders > capacity_cylinders ) {
            capacity_cylinders = glm::max(capacity_cylinders*2, nr_cylinders);
            updatePrimBufSize(buf_cylinders, nr_cylinders);
        }
        glBindBuffer(GL_ARRAY_BUFFER, buf_cylinders);
        glBufferSubData(GL_ARRAY_BUFFER, 0, nr_cylinders*pi_size, cylinders.data());
        glUnmapBuffer(GL_ARRAY_BUFFER);
    }
}
void AppBaseCanvas3d::drawInstance() const
{
    glBindVertexArray(ms_quad.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_quad.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_quad.nr_tris*3, GL_UNSIGNED_INT, nullptr, nr_quads);

    glBindVertexArray(ms_sphere.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_sphere.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_sphere.nr_tris*3, GL_UNSIGNED_INT, nullptr, nr_spheres);

    glBindVertexArray(ms_cylinder.vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms_cylinder.buf_tris);
	glDrawElementsInstanced(GL_TRIANGLES, ms_cylinder.nr_tris*3, GL_UNSIGNED_INT, nullptr, nr_cylinders);
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
    vp.drawImGuiAndUpdateCam();
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
        mtx_Model = glm::translate(p) * glm::rotate(angle, axis) * glm::scale(s);
    else 
        mtx_Model = glm::translate(p) * glm::scale(s);
    
    quads.push_back({mtx_Model, vec4(color, alpha)});
    nr_quads++;
}


void AppBaseCanvas3d::drawSphere(const vec3& p, const float w, const vec3& color, const float alpha) const {
    mat4 mtx_Model = glm::translate(p) * glm::scale(vec3(w));

    spheres.push_back({mtx_Model, vec4(color, alpha)});
    nr_spheres++;
}


void AppBaseCanvas3d::drawCylinder( const vec3& p1, const vec3& p2, const float w, const vec3& color, const float alpha ) const {
    vec3 diff = p1 - p2;
    vec3 mid = (p1 + p2) * 0.5f;
    vec3 s = {w, length(diff), w};
    vec3 axis = cross(vec3{0,1,0}, diff); // rotate 내부에서 normalize함
    float l = length(axis);
    float angle = atan2f(l, diff.y);
    mat4 mtx_Model;
    if( l>0.0000001f )
        mtx_Model = glm::translate(mid) * glm::rotate(angle, axis) * glm::scale(s);
    else 
        mtx_Model = glm::translate(mid) * glm::scale(s);
    
    cylinders.push_back({mtx_Model, vec4(color, alpha)});
    nr_cylinders++;
}


void AppBaseCanvas3d::drawQuad(const mat4& mtx, const vec3& color, const float alpha) const {
    quads.push_back({mtx, vec4(color, alpha)});
    nr_quads++;
}
void AppBaseCanvas3d::drawSphere(const mat4& mtx, const vec3& color, const float alpha) const {
    spheres.push_back({mtx, vec4(color, alpha)});
    nr_spheres++;
}
void AppBaseCanvas3d::drawCylinder( const mat4& mtx, const vec3& color, const float alpha ) const {
    cylinders.push_back({mtx, vec4(color, alpha)});
    nr_cylinders++;
}