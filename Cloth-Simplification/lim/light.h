//
//  2022-09-05 / im dong ye
// 
//	position에서 vec3(0)(중앙바닥)을 바라보는 direction light
// 
//	point light를 만든다면
//  draw 함수포인터에서 light의 위치를 받아서 모델자신을 바라보는 viewMat을 계산해서 적용해줘야하나?
//	그럼 이때 알맞은 fov는 뭐지?
//
//	only one light
//

#ifndef LIGHT_H
#define LIGHT_H

#include "limclude.h"

namespace lim
{
	class Light // direction
	{
	public:
		const GLuint shadowMapSize = 1024;
		static Program shadowProg;
	public:
		glm::vec3 position;
		glm::vec3 color;
		float intensity;
	public:
		TxFramebuffer shadowMap;
		glm::mat4 viewMat;
		glm::mat4 projMat;
	public:
		Light(glm::vec3 _pos={0.26, 2, 1}, glm::vec3 _color={1,1,1}, float _intensity = 1)
			:position(_pos), color(_color), intensity(_intensity), shadowMap()
		{
			if( shadowProg.ID==0 )
			{
				shadowProg.attatch("shader/const.vs").attatch("shader/const.fs").link();
			}
			shadowMap.resize(shadowMapSize, shadowMapSize);
			updateViewMat();
			/* fov 1.0은 60도 정도 2에서 1~-1사이의 중앙모델만 그린다고 가정하면 far을 엄청 멀리까지 안잡아도되고
			   depth의 4바이트 깊이를 많이 사용할수있다. */
			   //projMat = glm::perspective(1.0f, 1.0f, 0.01f, 2.f);
			projMat = glm::ortho(-2.f, 2.f, -2.f, 2.f, 0.01f, 4.f);
		}
		~Light() = default;
		void updateViewMat()
		{
			viewMat = glm::lookAt(position, {0,0,0}, {0,1,0});
		}
		/* light 와 model의 circular dependency때문에 shadowMap을 직접 그리는 함수를 외부에서 정의하게함. */
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