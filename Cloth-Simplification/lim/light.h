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
		static Program shadowProg;
	public:
		TxFramebuffer shadowMap;
		glm::vec3 position;
		glm::vec3 color;
		float intensity;
		glm::mat4 viewMat;
		glm::mat4 projMat;
	public:
		Light(glm::vec3 _pos ={40, 300, 150}, glm::vec3 _color ={1,1,1}, float _intensity = 1)
			:position(_pos), color(_color), intensity(_intensity), shadowMap()
		{
			if( shadowProg.ID==0 )
			{
				shadowProg.attatch("shader/const.vs").attatch("shader/const.fs").link();
			}
			shadowMap.resize(shadowMapSize, shadowMapSize);
		}
		~Light() = default;

		void drawShadowMap(std::function<void(GLuint shadowProgID)> drawModelsMeshWithModelMat)
		{
			shadowMap.bind();
			GLuint pid = shadowProg.use();
			setUniform(pid, "viewMat", viewMat);
			setUniform(pid, "projMat", projMat);

			drawModelsMeshWithModelMat(pid);

			shadowMap.unbind();
		}

	};
	Program Light::shadowProg("shadowMap");
}
#endif