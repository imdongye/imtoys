
#include "appbase_canvas3d.h"
#include <limbrary/limgui.h>
#include <imgui.h>

using namespace lim;
using namespace glm;
using namespace std;




AppBaseCanvas3d::AppBaseCanvas3d(int winWidth, int winHeight, const char* title, bool vsync
    , int nrMaxQuads, int nrMaxSpheres, int nrMaxCylinders) 
    : AppBase(winWidth, winHeight, title, vsync)
    , prog("lit_color")
    , vp("canvas3d", new FramebufferMs())
    , light()
    , ms_quad(true, false)
    , ms_sphere(1, true, false)
    , ms_cylinder(1.f, 1.f, 10, true, false)
{
    quads.reserve(nrMaxQuads);
    spheres.reserve(nrMaxSpheres);
    cylinders.reserve(nrMaxCylinders);

    prog.attatch("smvp.vs").attatch("canvas3d.fs").link();
    vp.camera.pivot = vec3(0, 1.0, 0);
    vp.camera.pos = vec3(0, 1.5, 3.4);
	vp.camera.updateViewMat();
    light.setShadowEnabled(true);
    LimGui::LightDirectionalEditorReset("d_light editor##canvas3d", "d_light shadow map##canvas3d");
}
AppBaseCanvas3d::~AppBaseCanvas3d()
{
}


void AppBaseCanvas3d::updateInstance() const
{

}
void AppBaseCanvas3d::drawInstance() const
{

}

void AppBaseCanvas3d::update()
{
    glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    canvasUpdate();

    nr_quads=0;
    nr_spheres=0;
    nr_cylinders=0;
    quads.clear();
    spheres.clear();
    cylinders.clear();
    canvasDraw();
    updateInstance();
    

    if( light.shadow ) {
        light.bakeShadowMap([&]() {
            drawInstance();
        });
    }

	vp.getFb().bind();
	prog.use();
	vp.camera.setUniformTo(prog);
	light.setUniformTo(prog);
	
    {
		drawSphere(light.tf.pos, vec3(1));
        vec3 p1 = light.tf.pos;
        vec3 p2 = p1+light.tf.dir*-0.6f;
        drawCylinder(p1, p2, vec3(1));
	}

    drawInstance();

	vp.getFb().unbind();
}
void AppBaseCanvas3d::updateImGui() {
    ImGui::DockSpaceOverViewport();
    vp.drawImGui();
    LimGui::LightDirectionalEditor(light);
    canvasImGui();
}



void AppBaseCanvas3d::drawQuad( const vec3& p, const vec3& n, const vec3& color, const vec2& sz ) const {
    vec3 s = {sz.x, sz.y, 1.f};
    vec3 axis = cross(vec3{0,0,1}, n);
    float l = length(axis);
    float angle = atan2f(l, n.z);
    mat4 mtx_Model;
    if( l>0.0000001 )
        mtx_Model = translate(p) * rotate(angle, axis) * scale(s);
    else 
        mtx_Model = translate(p) * scale(s);
    
    nr_quads++;
    quads.emplace_back(mtx_Model, vec4(color, 1));
}

void AppBaseCanvas3d::drawSphere( const vec3& p, const vec3& color, const float r ) const {
    mat4 mtx_Model = translate(p) * scale(vec3(r));

    nr_spheres++;
    spheres.emplace_back(mtx_Model, vec4(color, 1));
}

void AppBaseCanvas3d::drawCylinder( const vec3& p1, const vec3& p2, const vec3& color, const float r ) const {
    vec3 diff = p1 - p2;
    vec3 mid = (p1 + p2) * 0.5f;
    vec3 s = {r, length(diff), r};
    vec3 axis = cross(vec3{0,1,0}, diff);
    float l = length(axis);
    float angle = atan2f(l, diff.y);
    mat4 mtx_Model;
    if( l>0.0000001 )
        mtx_Model = translate(mid) * rotate(angle, axis) * scale(s);
    else 
        mtx_Model = translate(mid) * scale(s);
    
    nr_cylinders++;
    cylinders.emplace_back(mtx_Model, vec4(color, 1));
}