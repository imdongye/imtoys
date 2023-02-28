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

#ifndef SCENE_H
#define SCENE_H

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
		Scene(Light& _light, bool addGround=true): light(_light)
		{
			if( sceneCounter==0 ) {
				groundProgram = new Program("Ground");
				groundProgram->attatch("pos.vs").attatch("amiga_ground.fs").link();

				groundMesh = MeshGenerator::genQuad();
				groundMesh->color = glm::vec3(0.8, 0.8, 0); // yello ground			
			}

			if( addGround ) {
				ground = new Model(groundMesh, "ground", groundProgram);
				ground->position = glm::vec3(0, 0, 0);
				ground->scale = glm::vec3(100, 100, 1);
                ground->rotation = glm::angleAxis(F_PI*0.5f, glm::vec3(1,0,0));
				ground->updateModelMat();
				models.push_back(ground);
			}

			sceneCounter++;
		}
		virtual ~Scene()
		{
			sceneCounter--;

			if( sceneCounter==0 ) {
				delete groundProgram;
				delete ground;
			}
		}
	public:
		void setModel(Model* _model)
		{
			if( model!=nullptr )
				models.erase(std::find(models.begin(), models.end(), model));
			model = _model;
			ground->program = model->program;
			if( model!=nullptr )
				models.push_back(_model);
		}
		/* framebuffer직접설정해서 렌더링 */
		void render(GLuint fbo, GLuint width, GLuint height, Camera* camera)
		{
			if( light.shadowEnabled ) drawShadowMap();

			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glViewport(0, 0, width, height);
			glEnable(GL_DEPTH_TEST);
			glClearColor(0, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			camera->aspect = width/(float)height;
			camera->updateProjMat();

			drawModels(camera);
		}
		void render(Framebuffer* framebuffer, Camera* camera)
		{
			if( light.shadowEnabled ) drawShadowMap();

			camera->aspect = framebuffer->aspect;
			camera->updateProjMat();

			framebuffer->bind();
			drawModels(camera);
			framebuffer->unbind();
		}
	private:
		inline void drawModels(Camera* camera)
		{
			for( Model* model : models ) {
				if( model==nullptr ) continue;
				model->draw(*camera, light);
			}
		}
		void drawShadowMap()
		{
			// todo
			light.drawShadowMap([&](GLuint shadowProgID) {
				for( Model* model : models ) {
					setUniform(shadowProgID, "modelMat", model->model_mat);

					for( Mesh* mesh : model->meshes )
						mesh->draw(0); // only draw
				}
			});
		}
	};
}
#endif
