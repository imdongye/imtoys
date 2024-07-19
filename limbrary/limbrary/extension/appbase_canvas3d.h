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
        Program prog;
        LightDirectional light;

        MeshQuad quad;
		MeshIcoSphere sphere;
        MeshCylinder cylinder;
        int nr_quads;
        int nr_spheres;
        int nr_cylinders;
        std::vector<glm::vec4> quad_cols;
        std::vector<glm::mat4> quad_mtxs;
        std::vector<glm::vec4> sphere_cols;
        std::vector<glm::mat4> sphere_mtxs;
        std::vector<glm::vec4> cylinder_cols;
        std::vector<glm::mat4> cylinder_mtxs;

    protected:
        ViewportWithCamera vp;
    public:
        AppBaseCanvas3d(int winWidth=1280, int winHeight=720, const char* title="nonamed"
            , bool vsync=true, int nrMaxQuads=10, int nrMaxSpheres=10000, int nrMaxCylinders=1000);
        virtual ~AppBaseCanvas3d();

        virtual void update() final;
		virtual void updateImGui() final;

        virtual void canvasUpdate()=0;
        virtual void canvasDraw()const =0;
        virtual void canvasImGui()=0;

        void drawQuad( const glm::vec3& p, const glm::vec3& n, const glm::vec3& color, const glm::vec2& sz={100, 100} ) const;
        void drawSphere( const glm::vec3& p, const glm::vec3& color, const float r=0.02 ) const;
        void drawCylinder( const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, const float r=0.01 ) const;
    };
};

#endif