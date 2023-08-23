//
//	2022-08-24 / im dong ye
//
//	처음엔 imgui로 보여주기위한 renderTex를 생성하기위해 만들었다.
//	mass를 적용하기위해 intermediateFbo를 만들었고 msFbo에 draw하고
//	intermediateFbo에 blit 해주는 과정을 밖에서 해줘야한다.
// 
//	MSAA frame buffer
//	from : https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing
//
//	생성자에서 가상함수 쓰지말것. 생성되지않은 클래스의 함수를 호출하게됨.
//	glTexImage는 텍스쳐 크기를 동적으로 바꿀수있다고 명세되어있지만 문제가 있다고함.
//	=> 그냥 del후 gen해서 다시 bind 해주자.
// 
//	todo:
//	1. 생성
//	2. bind등 매 frame에 실행되는 민감한함수들 v table에 참조로 성능안좋아질거같은데 최적화 필요함.
//

#ifndef __framebuffer_h_
#define __framebuffer_h_

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace lim
{
	class Framebuffer
	{
	public:
		GLuint fbo, color_tex;
		glm::vec4 clear_color;
		GLuint width, height;
		float aspect;
	public:
		Framebuffer();
		virtual ~Framebuffer();
	protected:
		void genGlFboColor();
		virtual void initGL();
	public:
		bool resize(GLuint _width, GLuint _height=0);
		virtual void bind() const;
		virtual void unbind() const;
		/* ms framebuffer return intermidiate */
		virtual GLuint getRenderedTex() const;
	};

	class TexFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_tex;
	public:
		TexFramebuffer();
		virtual ~TexFramebuffer() override;
	protected:
		void genGLDepthTex();
		virtual void initGL() override;
	public:
		virtual void bind() const override;
	};

	class RboFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_rbo;
	public:
		RboFramebuffer();
		virtual ~RboFramebuffer();
	protected:
		void genGLDepthRbo();
		virtual void initGL() override;
	public:
		virtual void bind() const override;
	};

	class MsFramebuffer: public RboFramebuffer
	{
	public:
		const int samples = 8;
		Framebuffer intermediate_fb;
	public:
		MsFramebuffer();
		virtual ~MsFramebuffer();
	protected:
		void genGLFboMs();
		virtual void initGL() override;
	public:
		virtual void bind() const override;
		virtual void unbind() const override;
		virtual GLuint getRenderedTex() const override;
	};
}

#endif