#include <limbrary/model_view/scene.h>
#include <limbrary/model_view/code_mesh.h>
#include <limbrary/utils.h>



namespace lim
{
	Scene::Scene(Light& _light, bool addGround): light(_light)
	{
		if( sceneCounter==0 ) {
			groundProgram = new Program("Ground");
			groundProgram->attatch("pos.vs").attatch("amiga_ground.fs").link();

			groundMesh = code_mesh::genQuad();
			groundMesh->color = glm::vec3(0.8, 0.8, 0); // yello ground			
		}

		if( addGround ) {
			ground = new Model(groundMesh, "ground", groundProgram);
			ground->position = glm::vec3(0, 0, 0);
			ground->scale = glm::vec3(100, 100, 1);
			ground->orientation = glm::angleAxis(F_PI*0.5f, glm::vec3(1,0,0));
			ground->updateModelMat();
			models.push_back(ground);
		}

		sceneCounter++;
	}
	Scene::~Scene()
	{
		sceneCounter--;

		if( sceneCounter==0 ) {
			delete groundProgram;
			delete ground;
		}
	}
	void Scene::setModel(Model* _model)
	{
		if( model!=nullptr )
			models.erase(std::find(models.begin(), models.end(), model));
		model = _model;
		ground->program = model->program;
		if( model!=nullptr )
			models.push_back(_model);
	}
	/* framebuffer직접설정해서 렌더링 */
	void Scene::render(GLuint fbo, GLuint width, GLuint height, Camera* camera)
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
	void Scene::render(Framebuffer* framebuffer, Camera* camera)
	{
		if( light.shadowEnabled ) drawShadowMap();

		camera->aspect = framebuffer->aspect;
		camera->updateProjMat();

		framebuffer->bind();
		drawModels(camera);
		framebuffer->unbind();
	}
	void Scene::drawModels(Camera* camera)
	{
		for( Model* md : models ) {
			if( md==nullptr ) continue;
			md->draw(*camera, light);
		}
	}
	void Scene::drawShadowMap()
	{
		// todo
		light.drawShadowMap([&](GLuint shadowProgID) {
			for( Model* md : models ) {
				setUniform(shadowProgID, "modelMat", md->model_mat);

				for( Mesh* ms : model->meshes )
					ms->draw(0); // only draw
			}
		});
	}
}
