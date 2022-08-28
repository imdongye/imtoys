//
//	2022-08-26 / im dong ye
//
//	TODO list:
//  1. 바닥으로 0으로?
//

#ifndef SCENE_FIRST_H
#define SCENE_FIRST_H

#include "scene.h"

namespace lim
{

	class FirstScene: public Scene
	{
	private:
		Model* ground=nullptr;
		Model* model=nullptr;
	public:
		void alignGround()
		{
			ground->position = glm::vec3(0, -model->getBoundarySize().y*model->scale.y*0.5f, 0);
			ground->updateModelMat();
		}
		void loadModel(const char* path)
		{
			if( model!=nullptr )
			{
				models.erase(std::find(models.begin(), models.end(), model));
				delete model;
			}
			model = new Model(path);
			model->setUnitScaleAndPivot();
			model->updateModelMat();
			models.push_back(model);
			alignGround();
		}
	public:
		FirstScene()
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
			models.push_back(ground);

			loadModel("archive/meshes/stanford-bunny.obj");
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