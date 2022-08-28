//
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. init destroy 생성자소멸자 사용
//  3. 여러 program 적용
//
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


	class FirstScene: public Scene
	{
	public:
		FirstScene()
		{
			Program* heapProg = new Program();
			Program& refProg = *heapProg;
			refProg.attatch("shader/diffuse.vs").attatch("shader/diffuse.fs").link();
			programs.push_back(heapProg);

			//models.push_back(new Model("archive/backpack/backpack.obj"));
			//models.push_back(new Model("archive/nanosuit/nanosuit.obj"));
			//models.push_back(new Model("archive/tests/armadillo.obj"));
			models.push_back(new Model("archive/tests/igea.obj"));
			//models.push_back(new Model("archive/tests/teapot.obj"));
			//models.push_back(new Model("archive/tests/stanford-bunny.obj"));
			//models.push_back(new Model("archive/dwarf/Dwarf_2_Low.obj"));
			//models.push_back(new Model("archive/nanosuit/nanosuit.obj"));
			Model& m = *models.back();
			m.setUnitScaleAndPivot();
			m.updateModelMat();

			models.push_back(new Model([](std::vector<lim::n_model::Vertex>& vertices
							 , std::vector<GLuint>& indices
							 , std::vector<Texture>& textures) {
								 const float half = 100.0;
								 vertices.push_back({{-half, 0, half},
													{0, 1, 0}});
								 vertices.push_back({{half, 0, half}, {0, 1, 0}});
								 vertices.push_back({{half, 0, -half}, {0, 1, 0}});
								 vertices.push_back({{-half, 0, -half}, {0, 1, 0}});

								 indices.insert(indices.end(), {0,1,3});
								 indices.insert(indices.end(), {1,2,3});
							 }, "ground"));
			Model& ground = *models.back();
			ground.position = glm::vec3(0, -m.getBoundarySize().y*m.scale.y*0.5f, 0);
			ground.updateModelMat();
		}
		~FirstScene()
		{
			for( Model* model : models )
				delete model;
			for( Program* programs : programs )
				delete programs;
		}
		virtual void update() override
		{

		}
		virtual void render(GLuint fbo, GLuint width, GLuint height, Camera* camera) override
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
	};
}
#endif