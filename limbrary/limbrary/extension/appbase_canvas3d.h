/*
	simple static mesh canvas app template
	2024-07-01 / im dong ye

    todo:
    1. ssbo to VBO for opengl4.1(for mac), From https://www.youtube.com/watch?v=TOPvFvL_GRY

    note:
    update      -> render
    updateImGui -> renderImGui
*/
#ifndef __appbase_canvas3d_h_
#define __appbase_canvas3d_h_

#include <limbrary/application.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/camera_man.h>

namespace lim
{
    class AppBaseCanvas3d : public AppBase
    {
    private:
        struct PrimInfo {
            glm::mat4 mtx; // 64
            glm::vec4 col; // 16
        };
    private:
        Program prog;
        Program prog_shadow;
        LightDirectional light;

        MeshQuad ms_quad;
		MeshIcoSphere ms_sphere;
        MeshCylinder ms_cylinder;
        mutable int nr_quads,       capacity_quads;
        mutable int nr_spheres,     capacity_spheres;
        mutable int nr_cylinders,   capacity_cylinders;
        mutable std::vector<PrimInfo> quads;
        mutable std::vector<PrimInfo> spheres;
        mutable std::vector<PrimInfo> cylinders;

        GLuint buf_quads;
        GLuint buf_spheres;
        GLuint buf_cylinders;

    protected:
        ViewportWithCamera vp;
    public:
        AppBaseCanvas3d(int winWidth=1280, int winHeight=720, const char* title="nonamed"
            , bool vsync=true, int capacityQuads=10, int capacitySpheres=1000, int capacityCylinders=1000);
        virtual ~AppBaseCanvas3d();

        virtual void canvasUpdate() = 0;
        virtual void canvasDraw() const = 0;
        virtual void canvasImGui() = 0;
        virtual void customDrawShadow( const glm::mat4& mtx_View, const glm::mat4& mtx_Proj ) const {};
        virtual void customDraw( const Camera& cam, const LightDirectional& lit ) const {};


        void drawQuad( const glm::vec3& p, const glm::vec3& n, const glm::vec2& sz, const glm::vec3& color, const float alpha=1.f ) const;
        void drawQuad( const glm::mat4& mtx, const glm::vec3& color, const float alpha=1.f ) const;
        void drawSphere( const glm::vec3& p, const float width, const glm::vec3& color, const float alpha=1.f ) const;
        void drawSphere( const glm::mat4& mtx, const glm::vec3& color, const float alpha=1.f ) const;
        void drawCylinder( const glm::vec3& p1, const glm::vec3& p2, const float width, const glm::vec3& color, const float alpha=1.f ) const;
        void drawCylinder( const glm::mat4& mtx, const glm::vec3& color, const float alpha=1.f ) const;
    private:
        void resetInstance();
        void updateInstance() const;
        void drawInstance() const;
        virtual void update() final;
		virtual void updateImGui() final;
    };
};

#endif