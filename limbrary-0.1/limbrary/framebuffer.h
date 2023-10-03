/*

2022-08-24 / im dong ye

처음엔 imgui로 보여주기위한 renderTex를 생성하기위해 만들었다.
mass를 적용하기위해 intermediateFbo를 만들었고 msFbo에 draw하고
intermediateFbo에 blit 해주는 과정을 밖에서 해줘야한다.

MSAA frame buffer
From : https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing

생성자에서 가상함수 쓰지말것. 생성되지않은 클래스의 함수를 호출하게됨.
glTexImage는 텍스쳐 크기를 동적으로 바꿀수있다고 명세되어있지만 문제가 있다고함.
=> 그냥 del후 gen해서 다시 bind 해주자.

Note:
you must use after resize

Todo:
1. 생성
2. bind등 매 frame에 실행되는 민감한함수들 v table에 참조로 성능안좋아질거같은데 최적화 필요함.

*/

#ifndef __framebuffer_h_
#define __framebuffer_h_

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace lim
{
	// depth attachment 없음
	class Framebuffer
	{
	public:
		GLuint fbo = 0;
		GLuint color_tex = 0;
		glm::vec4 clear_color = {0.05f, 0.09f, 0.11f, 1.0f};
		GLuint width = 0;
		GLuint height = 0;
		float aspect = 1.f;
	private:
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
	public:
		Framebuffer();
		Framebuffer(Framebuffer&& src) noexcept;
		Framebuffer& operator=(Framebuffer&& src) noexcept;
		virtual ~Framebuffer() noexcept;
	protected:
		virtual void initGL();
		void genGlFboColor();
	public:
		virtual void bind() const;
		virtual void unbind() const;
		/* ms framebuffer return intermidiate */
		virtual GLuint getRenderedTex() const;
		bool resize(GLuint _width, GLuint _height);
		bool resize(GLuint square);
	};

	// depth_tex 샘플링 가능, 성능저하
	class TexFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_tex = 0;
	private:
		TexFramebuffer(const TexFramebuffer&) = delete;
		TexFramebuffer& operator=(const TexFramebuffer&) = delete;
	public:
		TexFramebuffer();
		TexFramebuffer(TexFramebuffer&& src) noexcept;
		TexFramebuffer& operator=(TexFramebuffer&& src) noexcept;
		virtual ~TexFramebuffer() noexcept override;
	protected:
		virtual void initGL() override;
		void genGLDepthTex();
	public:
		virtual void bind() const override;
	};

	// depth_rbo 샘플링 불가능, 성능향상
	class RboFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_rbo = 0;
	private:
		RboFramebuffer(const RboFramebuffer&) = delete;
		RboFramebuffer& operator=(const RboFramebuffer&) = delete;
	public:
		RboFramebuffer();
		RboFramebuffer(RboFramebuffer&& src) noexcept;
		RboFramebuffer& operator=(RboFramebuffer&& src) noexcept;
		virtual ~RboFramebuffer() noexcept override;
	protected:
		virtual void initGL() override;
		void genGLDepthRbo();
	public:
		virtual void bind() const override;
	};

	// 멀티셈플링으로 안티엘리어싱됨
	class MsFramebuffer: public RboFramebuffer
	{
	private:
		int samples = 8;
		Framebuffer intermediate_fb;
	private:
		MsFramebuffer(const MsFramebuffer&) = delete;
		MsFramebuffer& operator=(const MsFramebuffer&) = delete;
	public:
		MsFramebuffer(int samples = 8);
		MsFramebuffer(MsFramebuffer&& src) noexcept;
		MsFramebuffer& operator=(MsFramebuffer&& src) noexcept;
		virtual ~MsFramebuffer() noexcept override;
	protected:
		virtual void initGL() override;
		void genGLFboMs();
	public:
		virtual void bind() const override;
		virtual void unbind() const override;
		virtual GLuint getRenderedTex() const override;
	};
}

#endif