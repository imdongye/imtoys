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
		Model* ground=nullptr;
		Model* model=nullptr;
		std::vector<Model*> models;
		std::vector<Program*> programs;
	public:
		void alignGround()
		{
			if( model==nullptr ) return;
			ground->position = glm::vec3(0, -model->getBoundarySize().y*model->scale.y*0.5f, 0);
			ground->updateModelMat();
		}
		void loadModel(const char* path)
		{
			setModel(new Model(path));
		}
		void setModel(Model* _model)
		{
			if( model!=nullptr )
			{
				models.erase(std::find(models.begin(), models.end(), model));
				delete model;
			}
			model = _model;
			model->setUnitScaleAndPivot();
			model->updateModelMat();
			models.push_back(model);
			alignGround();
		}
	public:
		Scene()
		{
			Program& refProg = *(new Program());
			refProg.attatch("shader/diffuse.vs").attatch("shader/diffuse.fs").link();
			programs.push_back(&refProg);

			ground = new Model([](std::vector<lim::n_model::Vertex>& vertices
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
							   }, "ground");
			ground->position = glm::vec3(0, -1, 0);
			ground->updateModelMat();
			models.push_back(ground);
		}
		virtual ~Scene()
		{
			for( Model* model : models )
				delete model;
			for( Program* programs : programs )
				delete programs;
		}
		virtual void render(GLuint fbo, GLuint width, GLuint height, Camera* camera)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glViewport(0, 0, width, height);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);// z-buffer clipping

			GLuint loc, pid;
			Program& program = *programs[0];
			pid = program.use();

			loc = glGetUniformLocation(pid, "projMat");
			glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera->projMat));//&camera.projMat[0][0]);

			loc = glGetUniformLocation(pid, "viewMat"); // also get camera pos here
			glUniformMatrix4fv(loc, 1, GL_FALSE, value_ptr(camera->viewMat));

			loc = glGetUniformLocation(pid, "cameraPos");
			glUniform3fv(loc, 1, value_ptr(camera->position));

			// maybe vpMat
			// and modelMat is declare in model->draw

			for( Model* model : models )
			{
				model->draw(*programs[0]);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		void render(Viewport* vp)
		{
			if( vp->framebuffer->FBO==0 ) return;
			render(vp->framebuffer->FBO, vp->framebuffer->width, vp->framebuffer->height, vp->camera);
		}
	};
}
#endif