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
    class Canvas3dApp : public AppBase
    {
    private:
        Program prog;
        ViewportWithCamera vp;
        LightDirectional light;
        MeshQuad quad;
		MeshSphere sphere;
        MeshCylinder cylinder;

    public:
        Canvas3dApp(int winWidth=1280, int winHeight=720, const char* title="nonamed", bool vsync=true);
        virtual ~Canvas3dApp();

        virtual void update() final;
		virtual void updateImGui() final;
        virtual void render()=0;
        virtual void renderImGui()=0;

        void drawQuad( const glm::vec3& p, const glm::vec3& n, const glm::vec2& sz, const glm::vec4& color );
        void drawSphere( const glm::vec3& p, const float r, const glm::vec4& color );
        void drawCylinder( const glm::vec3& p1, const glm::vec3& p2, float r, const glm::vec4& color );
    };
};

#endif