//
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. init destroy 생성자소멸자 사용
//  3. 여러 program 적용
//

#ifndef SCENE_H
#define SCENE_H

#include "model.h";
#include "program.h"
#include "camera.h"
#include "viewport.h"

namespace lim
{
	class Scene
	{
	public:
		std::vector<Model*> models;
		std::vector<Program*> programs;
	public:
		Scene() = default;
		virtual ~Scene() = default;
		virtual void update()=0;
		virtual void render(GLuint fbo, GLuint width, GLuint height, Camera* camera)=0;
		void render(Viewport* vp)
		{
			if( vp->framebuffer->FBO==0 ) return;
			render(vp->framebuffer->FBO, vp->framebuffer->width, vp->framebuffer->height, vp->camera);
		}
	};
}
#endif