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
#include <limbrary/model_view/light.h>
#include <limbrary/framebuffer.h>
#include <glm/gtx/transform.hpp>


namespace lim
{
	Light::Light(float _yaw, float _pitch, glm::vec3 _color, float _intensity, float shadowEnabled)
		:yaw(_yaw), pitch(_pitch), color(_color), intensity(_intensity), shadowMap(), shadowEnabled(true)
	{
		if( ref_count++==0 ) {
			shadowProg = new Program("shadow program");
			shadowProg->attatch("pos.vs").attatch("depth.fs").link();
		}

		shadowMap.clear_color = glm::vec4(1);
		shadowMap.resize(shadowMapSize, shadowMapSize);

		// fov 1.0은 60도 정도 2에서 1~-1사이의 중앙모델만 그린다고 가정하면 far을 엄청 멀리까지 안잡아도되고
		// depth의 4바이트 깊이를 많이 사용할수있다.
		// projMat = glm::perspective(1.f, 1.f, shadowZNaer, shadowZFar);

		const float halfW = orthoWidth*0.5f;
		const float halfH = orthoHeight*0.5f;
		projMat = glm::ortho(-halfW, halfW, -halfH, halfH, shadowZNaer, shadowZFar);
		updateMembers();
	}
	Light::~Light()
	{
		if( --ref_count==0 ) {
			delete shadowProg;
		}
	}
	void Light::updateMembers()
	{
		yaw = glm::mod(yaw, 360.f);
		//const float minPitchDegree = 20;
		pitch = glm::clamp(pitch, 20.f, 89.f);
		position = glm::vec3(glm::rotate(glm::radians(yaw), glm::vec3(0, 1, 0))
								* glm::rotate(glm::radians(-pitch), glm::vec3(1, 0, 0))
								* glm::vec4(0, 0, distance, 1));
		direction = glm::normalize(position);

		viewMat = glm::lookAt(position, {0,0,0}, {0,1,0});
		vpMat = projMat * viewMat;
	}
	void Light::setUniforms(const Program& prog) const
	{
		prog.setUniform("lightDir", direction);
		prog.setUniform("lightColor", color);
		prog.setUniform("lightInt", intensity);
		prog.setUniform("lightPos", position);

		if( shadowEnabled ) {
			prog.setUniform("shadowEnabled", 1);
			prog.setUniform("shadowVP", vpMat);
			/* slot을 shadowMap은 뒤에서 부터 사용 texture은 앞에서 부터 사용 */
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, shadowMap.color_tex);
			prog.setUniform("shadowMap", 31);
		}
		else {
			prog.setUniform("shadowEnabled", -1);
		}
	}
	/* light 와 model의 circular dependency때문에 shadowMap을 직접 그리는 함수를 외부에서 정의하게함. */
	// todo: solve circular dependency with inl file
	void Light::drawShadowMap(std::function<void(GLuint shadowProgID)> drawModelsMeshWithModelMat)
	{
		shadowMap.bind();

		shadowProg->use();
		shadowProg->setUniform("viewMat", viewMat);
		shadowProg->setUniform("projMat", projMat);

		drawModelsMeshWithModelMat(shadowProg->pid);

		shadowMap.unbind();
	}
}
