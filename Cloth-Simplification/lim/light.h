//
//  2022-09-05 / im dong ye
//
//	only one light
//

#ifndef LIGHT_H
#define LIGHT_H

#include "limclude.h"

namespace lim
{
	class Light
	{
	private:
		const GLuint shadowMapSize = 1024;
	public:
		glm::vec3 position;
		glm::vec3 color;
		float intensity;
		TxFramebuffer shadowMap;
		glm::mat4 viewMat;
		glm::mat4 projMat;
	public:
		Light(glm::vec3 _pos ={40, 300, 150}, glm::vec3 _color ={1,1,1}, float _intensity = 1)
			:position(_pos), color(_color), intensity(_intensity)
		{
			shadowMap.resize(shadowMapSize, shadowMapSize);
		}
		~Light() = default;
	public:
		void drawShadowMap(std::vector<Model*> models)
		{
			shadowMap.bind();
			for( Model* model : models )
			{
				GLuint pid;
				pid = model->program->use();

				setUniform(pid, "viewMat", viewMat);
				setUniform(pid, "projMat", projMat);
				setUniform(pid, "modelMat", model->modelMat);

				for( Mesh* mesh : model->meshes )
					mesh->draw(pid);
			}
			shadowMap.unbind();
		}

	};
}
#endif