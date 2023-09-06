#include <limbrary/model_view/scene.h>
#include <limbrary/model_view/code_mesh.h>
#include <limbrary/utils.h>



namespace lim
{
	Scene::Scene()
	{
	}
	Scene::~Scene()
	{
	}
	/* framebuffer직접설정해서 렌더링 */
	void Scene::render(GLuint fbo, GLuint width, GLuint height, Camera* camera)
	{
		const Light& light = *lights[0];
		if( light.shadow_enabled ) light.drawShadowMap(models);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		drawModels(camera);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void Scene::render(Framebuffer* framebuffer, Camera* camera)
	{
		const Light& light = *lights[0];
		if( light.shadow_enabled ) light.drawShadowMap(models);

		framebuffer->bind();
		drawModels(camera);
		framebuffer->unbind();
	}
	void Scene::drawModels(Camera* camera)
	{
		for( Model* md : models ) {
			if( md==nullptr ) continue;
			drawModel(*camera, *md);
		}
	}
	void Scene::drawModel(const Camera& camera, const Model& model) {
		if( model.program==nullptr ) {
			log::err("model has not progam\n");
			return;
		}
		const Program& prog = *model.program;
		const Light& light = *lights[0];
		prog.use();
		prog.setUniform("cameraPos", camera.position);
		prog.setUniform("projMat", camera.proj_mat);
		prog.setUniform("viewMat", camera.view_mat);
		prog.setUniform("modelMat", model.model_mat);
		prog.setUniform("bumpHeight", model.bumpHeight);
		prog.setUniform("texDelta", model.texDelta);
		prog.setUniform("lightDir", light.direction);
		prog.setUniform("lightColor", light.color);
		prog.setUniform("lightInt", light.intensity);
		prog.setUniform("lightPos", light.position);
		prog.setUniform("shadowEnabled", (light.shadow_enabled)?1:0);
		if( light.shadow_enabled ) {
			prog.setUniform("shadowVP", light.vp_mat);
			/* slot을 shadowMap은 뒤에서 부터 사용 texture은 앞에서 부터 사용 */
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, light.shadow_map.color_tex);
			prog.setUniform("shadowMap", 31);
		}
		for( int i=0; i<model.meshes.size(); i++ ) {
			const Mesh& mesh = *model.meshes[i];

			int slotCounter = 0;
			GLuint diffuseNr = 0;
			GLuint specularNr = 0;
			GLuint normalNr = 0;
			GLuint ambientNr = 0;
			for (std::shared_ptr<Texture> tex : mesh.textures)
			{
				std::string &type = tex->tag;
				// uniform samper2d nr is start with 0
				int backNum = 0;
				if (type == "map_Kd")
					backNum = diffuseNr++;
				else if (type == "map_Ks")
					backNum = specularNr++;
				else if (type == "map_Bump")
					backNum = normalNr++;
				else if (type == "map_Ka")
					backNum = ambientNr++;

				std::string varName = type + std::to_string(backNum);
				glActiveTexture(GL_TEXTURE0 + slotCounter); // slot
				glBindTexture(GL_TEXTURE_2D, tex->tex_id);
				prog.setUniform(varName.c_str(), slotCounter++); // to sampler2d
			}
			prog.setUniform("texCount", (int)mesh.textures.size());
			prog.setUniform("Kd", mesh.color);
		

			glBindVertexArray(mesh.VAO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
			glDrawElements(mesh.draw_mode, static_cast<GLuint>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
		}
	}
}
