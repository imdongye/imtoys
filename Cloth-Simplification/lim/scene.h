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
//

#ifndef SCENE_H
#define SCENE_H

#include "limclude.h"

namespace lim
{
	class Scene
	{
	public:
		Model* ground=nullptr;
		Model* model=nullptr; // main model
		std::vector<Model*> models;
		Light& light;
		bool enableShadow;
	public:
		Scene(Program* groundProgram, Light& _light): light(_light), enableShadow(true)
		{
			ground = new Model([](std::vector<lim::n_mesh::Vertex>& vertices
							   , std::vector<GLuint>& indices
							   , std::vector<Texture>& textures)
							   {
								   const float half = 100.0;
								   vertices.push_back({{-half, 0, half},
													   {0, 1, 0}});
								   vertices.push_back({{half, 0, half}, {0, 1, 0}});
								   vertices.push_back({{half, 0, -half}, {0, 1, 0}});
								   vertices.push_back({{-half, 0, -half}, {0, 1, 0}});

								   indices.insert(indices.end(), {0,1,3});
								   indices.insert(indices.end(), {1,2,3});
							   }, groundProgram, "ground");
			ground->position = glm::vec3(0, 0, 0);
			ground->updateModelMat();
			models.push_back(ground);
		}
		virtual ~Scene()
		{
			delete ground;
		}
	public:
		void setModel(Model* _model)
		{
			if( model!=nullptr )
			{
				models.erase(std::find(models.begin(), models.end(), model));
			}
			model = _model;
			models.push_back(_model);
		}
		void render(Camera* camera)
		{
			for( Model* model : models )
			{
				model->draw(*camera, light);
			}
		}
		void render(GLuint fbo, GLuint width, GLuint height, Camera* camera)
		{
			if( enableShadow )
			{
				light.drawShadowMap([&](GLuint shadowProgID) {
					for( Model* model : models )
					{
						setUniform(shadowProgID, "modelMat", model->modelMat);

						for( Mesh* mesh : model->meshes )
							mesh->draw();
					}
									});
			}

			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glEnable(GL_FRAMEBUFFER_SRGB);
			glViewport(0, 0, width, height);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			render(camera);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		void render(Viewport* vp)
		{
			Framebuffer& fb =  *(vp->framebuffer);
			if( fb.renderable() == false ) return;

			fb.bind();
			render(vp->camera);
			fb.unbind();
		}
	};
}
#endif