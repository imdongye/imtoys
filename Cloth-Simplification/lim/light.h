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

namespace lim
{
	class Light // direction
	{
	public:
		static Program shadowProg;
		const GLuint shadowMapSize = 1024;
		const float shadowZNaer = 3;
		const float shadowZFar = 6;
		const float orthoWidth = 4;
		const float orthoHeight = 8;
		const float distance = 5;
	public: // editable
		bool shadowEnabled; // 밖에서 확인하고 사용하기
		float yaw, pitch;
		glm::vec3 color;
		float intensity;
	public: // update result
		glm::vec3 position;
		glm::vec3 direction;
		TxFramebuffer shadowMap;
		glm::mat4 viewMat;
		glm::mat4 projMat;
		glm::mat4 vpMat;
	public:
		Light(float _yaw=58.f, float _pitch=51.f, glm::vec3 _color={1,1,1}, float _intensity = 1)
			:yaw(_yaw), pitch(_pitch), color(_color), intensity(_intensity), shadowMap(), shadowEnabled(true)
		{
			if( shadowProg.ID==0 )
			{
				shadowProg.attatch("shader/pos.vs").attatch("shader/depth.fs").link();
			}
			shadowMap.clearColor = glm::vec4(1);
			shadowMap.resize(shadowMapSize, shadowMapSize);

			/* fov 1.0은 60도 정도 2에서 1~-1사이의 중앙모델만 그린다고 가정하면 far을 엄청 멀리까지 안잡아도되고
			   depth의 4바이트 깊이를 많이 사용할수있다. */
			   //projMat = glm::perspective(1.0f, 1.0f, 0.01f, 2.f);
			const float halfW = orthoWidth*0.5f;
			const float halfH = orthoHeight*0.5f;
			projMat = glm::ortho(-halfW, halfW, -halfH, halfH, shadowZNaer, shadowZFar);
			//projMat = glm::perspective(1.f, 1.f, shadowZNaer, shadowZFar);
			updateMembers();
		}
		~Light() = default;
	public:
		void updateMembers()
		{
			yaw = glm::mod(yaw, 360.f);
			const float minPitchDegree = 20;
			pitch = glm::clamp(pitch, 20.f, 89.f);
			position = glm::vec3(glm::rotate(glm::radians(yaw), glm::vec3(0, 1, 0))
								 * glm::rotate(glm::radians(-pitch), glm::vec3(1, 0, 0))
								 * glm::vec4(0, 0, distance, 1));
			direction = glm::normalize(position);

			viewMat = glm::lookAt(position, {0,0,0}, {0,1,0});
			vpMat = projMat * viewMat;
		}
		void setUniforms(GLuint pid) const
		{
			setUniform(pid, "lightDir", direction);
			setUniform(pid, "lightColor", color);
			setUniform(pid, "lightInt", intensity);
			if( shadowEnabled )
			{
				setUniform(pid, "shadowEnabled", 1);
				setUniform(pid, "shadowVP", vpMat);
				/* slot을 shadowMap은 뒤에서 부터 사용 texture은 앞에서 부터 사용 */
				glActiveTexture(GL_TEXTURE31);
				glBindTexture(GL_TEXTURE_2D, shadowMap.colorTex);
				setUniform(pid, "shadowMap", 31);
			}
			else setUniform(pid, "shadowEnabled", -1);
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