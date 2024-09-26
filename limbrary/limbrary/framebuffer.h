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
glEnable로 사용하는 옵션은 콜러 세이브로 가정하므로 framebuffer bind하기전에 백업해두거나 원하는걸 다시 켜줘야한다.

Todo:
1. 생성
2. bind등 매 frame에 실행되는 민감한함수들 v table에 참조로 성능안좋아질거같은데 최적화 필요함.
3. 부모 deinit 중복 호출 문제
4. GL_DEPTH_STENCIL_ATTACHMENT 24+8

IFramebuffer 상속깊이는 무조건 한번이여야하고
자식소멸자에서 deinit을 호출해줘야함.


*/

#ifndef __framebuffer_h_
#define __framebuffer_h_

#include "texture.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <functional>
#include <limbrary/tools/mecro.h>

namespace lim
{
	class IFramebuffer : public NoCopyAndMove
	{
	public:
		GLuint fbo = 0;
		glm::ivec2 size;
		float aspect = 1.f;
		glm::vec4 clear_color = {0.05f, 0.09f, 0.11f, 1.0f};
		bool blendable = false;

	protected:
		IFramebuffer() = default;
	public:		
		// 자식 소멸자에서 deinit호출해줘야함.
		virtual ~IFramebuffer() = default;

		// height -1 is square
		void resize(const glm::ivec2& _size);

		void initGL();
		void deinitGL();
		void bind() const;
		void unbind() const;

		// ms framebuffer return intermidiate
		virtual GLuint getRenderedTexId() const = 0;
		
	protected:
		virtual void myInitGL() = 0;
		virtual void myDeinitGL() = 0;
		virtual void myBind() const = 0;
		virtual void myUnbind() const = 0;
	};



	class FramebufferNoDepth : public IFramebuffer
	{
	public:
		Texture color_tex;

	public:
		FramebufferNoDepth(int nrChannels = 3, int bitPerChannel = 8);
		~FramebufferNoDepth(); 

		float* makeFloatPixelsBuf() const;
		virtual GLuint getRenderedTexId() const final;

	protected:
		virtual void myInitGL() final;
		virtual void myDeinitGL() final;
		virtual void myBind() const final;
		virtual void myUnbind() const final;
	};



	class FramebufferOnlyDepth: public IFramebuffer
	{
	public:
		Texture depth_tex;

	public:
		FramebufferOnlyDepth();
		~FramebufferOnlyDepth();

		virtual GLuint getRenderedTexId() const final;

	protected:
		virtual void myInitGL() final;
		virtual void myDeinitGL() final;
		virtual void myBind() const final;
		virtual void myUnbind() const final;
	};



	// depth_tex 샘플링 가능, 성능저하
	class FramebufferTexDepth: public IFramebuffer
	{
	public:
		Texture color_tex;
		Texture depth_tex;

	public:
		FramebufferTexDepth(int nrChannels = 3, int bitPerChannel = 8);
		~FramebufferTexDepth();

		virtual GLuint getRenderedTexId() const final;

	protected:
		virtual void myInitGL() final;
		virtual void myDeinitGL() final;
		virtual void myBind() const final;
		virtual void myUnbind() const final;
	};



	// depth_rbo 샘플링 불가능, 성능향상
	class FramebufferRbDepth: public IFramebuffer
	{
	public:
		Texture color_tex;
		GLuint depth_rbo_id = 0;

	public:
		FramebufferRbDepth(int nrChannels = 3, int bitPerChannel = 8);
		~FramebufferRbDepth();

		virtual GLuint getRenderedTexId() const final;
		
	protected:
		virtual void myInitGL() final;
		virtual void myDeinitGL() final;
		virtual void myBind() const final;
		virtual void myUnbind() const final;
	};

	// 멀티셈플링으로 안티엘리어싱됨
	class FramebufferMs: public IFramebuffer
	{
	private:
		int samples = 8;
		FramebufferNoDepth intermediate_fb;
		glm::vec4 clear_color = {0.05f, 0.09f, 0.11f, 1.0f};
		GLuint ms_color_tex_id = 0;
		GLuint ms_depth_rbo_id = 0;

	public:
		FramebufferMs(int samples = 4, int nrChannels = 3, int bitPerChannel = 8);
		~FramebufferMs();

		virtual GLuint getRenderedTexId() const final;

	protected:
		virtual void myInitGL() final;
		virtual void myDeinitGL() final;
		virtual void myBind() const final;
		virtual void myUnbind() const final;
	};

}

#endif