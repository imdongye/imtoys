/*
	simple static mesh canvas app template
	2024-07-01 / im dong ye

    update      -> render
    updateImGui -> renderImGui
*/
#ifndef __canvas3d_app_h_
#define __canvas3d_app_h_

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
		MeshSphere sphere;
        MeshCylinder cylinder;
    protected:
        ViewportWithCamera vp;
    public:
        AppBaseCanvas3d(int winWidth=1280, int winHeight=720, const char* title="nonamed", bool vsync=true);
        virtual ~AppBaseCanvas3d();

        virtual void update() final;
		virtual void updateImGui() final;
        virtual void render()=0;
        virtual void renderImGui()=0;

        void drawQuad( const glm::vec3& p, const glm::vec3& n, const glm::vec3& color, const glm::vec2& sz={100, 100} ) const;
        void drawSphere( const glm::vec3& p, const glm::vec3& color, const float r=0.02 ) const;
        void drawCylinder( const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color, const float r=0.01 ) const;
    };
};

#endif