/*
	simple static mesh canvas app template
	2024-07-01 / im dong ye

    todo:
    1. instance draw mesh
    2. cacheing matrix buffer

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
        const int nr_max_quads;
        const int nr_max_spheres;
        const int nr_max_cylinders;
        mutable int nr_quads;
        mutable int nr_spheres;
        mutable int nr_cylinders;
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
            , bool vsync=true, int nrMaxQuads=10, int nrMaxSpheres=10000, int nrMaxCylinders=1000);
        virtual ~AppBaseCanvas3d();

        virtual void canvasUpdate() = 0;
        virtual void canvasDraw() const = 0;
        virtual void canvasImGui() = 0;

        void drawQuad( const glm::vec3& p, const glm::vec3& n, const glm::vec3& color, const glm::vec2& sz={100, 100} ) const;
        void drawSphere( const glm::vec3& p, const glm::vec3& color, const float w=0.05f ) const;
        void drawCylinder( const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, const float w=0.025f ) const;
    private:
        void resetInstance();
        void updateInstance() const;
        void drawInstance() const;
        virtual void update() final;
		virtual void updateImGui() final;
    };
};

#endif