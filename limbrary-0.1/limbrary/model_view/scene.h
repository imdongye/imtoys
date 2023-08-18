//
//	2022-08-26 / im dong ye
//
//	ground모델을 직접가지고 다른 모델들은 참조한다.
//	참조모델들과 light로 viewport나 framebuffer에 렌더링한다.
// 
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. init destroy 생성자소멸자 사용
//  3. 여러 program 적용
//	4. shared pointer 사용
//	5. 여러 라이트
//

#ifndef __scene_h_
#define __scene_h_

#include "model.h"
#include "../framebuffer.h"
#include "light.h"
#include "camera.h"
#include <vector>


namespace lim
{
	class Scene
	{
	private:
		inline static GLuint sceneCounter=0;
		inline static Program *groundProgram=nullptr;
		inline static Mesh *groundMesh = nullptr;
		friend class AppBase;
	public:
		Model* model=nullptr; // main model
		Model* ground=nullptr;
		std::vector<Model*> models;
		Light& light;
	public:
		Scene(Light& _light, bool addGround=true);
		virtual ~Scene();
	public:
		void setModel(Model* _model);
		/* framebuffer직접설정해서 렌더링 */
		void render(GLuint fbo, GLuint width, GLuint height, Camera* camera);
		void render(Framebuffer* framebuffer, Camera* camera);
	private:
		inline void drawModels(Camera* camera);
		void drawShadowMap();
	};
}
#endif
