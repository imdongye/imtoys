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
컬러버퍼는 8비트 3채널 고정

Todo:
1. 생성
2. bind등 매 frame에 실행되는 민감한함수들 v table에 참조로 성능안좋아질거같은데 최적화 필요함.
3. 부모 deinit 중복 호출 문제
4. GL_DEPTH_STENCIL_ATTACHMENT 24+8

*/

#ifndef __framebuffer_h_
#define __framebuffer_h_

#include "texture.h"
#include <glad/glad.h>
#include <glm/glm.hpp>


namespace lim
{
	// depth attachment 없음
	class Framebuffer
	{
	public:
		GLuint fbo = 0;
		Texture color_tex;
		glm::vec4 clear_color = {0.05f, 0.09f, 0.11f, 1.0f};
		GLuint width = 0;
		GLuint height = 0;
		float aspect = 1.f;
	public:
		Framebuffer(GLint interFormat = GL_RGB);
		Framebuffer(Framebuffer&& src) noexcept;
		Framebuffer& operator=(Framebuffer&& src) noexcept;
		virtual ~Framebuffer() noexcept;

		virtual void bind() const;
		virtual void unbind() const;
		/* ms framebuffer return intermidiate */
		virtual GLuint getRenderedTex() const;
		// height -1 is square
		bool resize(GLuint _width, GLuint _height=-1);
	protected:
		std::function<void()> initGL_hook;
		void initGL();
		std::function<void()> deinitGL_hook;
		void deinitGL();
	private:
		Framebuffer(const Framebuffer&) = delete;
		Framebuffer& operator=(const Framebuffer&) = delete;
	};

	// depth_tex 샘플링 가능, 성능저하
	class FramebufferTexDepth: public Framebuffer
	{
	public:
		Texture depth_tex;
	public:
		FramebufferTexDepth(GLint interFormat = GL_RGB);
		FramebufferTexDepth(FramebufferTexDepth&& src) noexcept;
		FramebufferTexDepth& operator=(FramebufferTexDepth&& src) noexcept;
		virtual ~FramebufferTexDepth() noexcept override;

		virtual void bind() const override;
	protected:
		void genGLDepthTex();
	private:
		FramebufferTexDepth(const FramebufferTexDepth&) = delete;
		FramebufferTexDepth& operator=(const FramebufferTexDepth&) = delete;
	};

	// depth_rbo 샘플링 불가능, 성능향상
	class FramebufferRbDepth: public Framebuffer
	{
	public:
		GLuint depth_rbo = 0;
	public:
		FramebufferRbDepth(GLint interFormat = GL_RGB);
		FramebufferRbDepth(FramebufferRbDepth&& src) noexcept;
		FramebufferRbDepth& operator=(FramebufferRbDepth&& src) noexcept;
		virtual ~FramebufferRbDepth() noexcept override;

		virtual void bind() const override;
	private:
		FramebufferRbDepth(const FramebufferRbDepth&) = delete;
		FramebufferRbDepth& operator=(const FramebufferRbDepth&) = delete;
	};

	// 멀티셈플링으로 안티엘리어싱됨
	class FramebufferMs: public FramebufferRbDepth
	{
	private:
		int samples = 8;
		Framebuffer intermediate_fb;
	public:
		FramebufferMs(GLint interFormat = GL_RGB, int samples = 8);
		FramebufferMs(FramebufferMs&& src) noexcept;
		FramebufferMs& operator=(FramebufferMs&& src) noexcept;
		virtual ~FramebufferMs() noexcept override;

		virtual void bind() const override;
		virtual void unbind() const override;
		virtual GLuint getRenderedTex() const override;
	private:
		FramebufferMs(const FramebufferMs&) = delete;
		FramebufferMs& operator=(const FramebufferMs&) = delete;
	};
}

#endif