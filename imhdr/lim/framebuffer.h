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
//	todo:
//	1. 부모소멸자 호출안하는 방법이 있나? framebuffer::clear두번호출
//	2. create 가상함수 재활용
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

namespace lim
{
	class Framebuffer
	{
	public:
		GLuint fbo, colorTex;
		glm::vec4 clearColor;
		GLuint width, height;
		float aspect;
	public:
		Framebuffer(): clearColor({0,0,1,1}), fbo(0), colorTex(0)
		{
			resize(32, 32);
		}
		virtual ~Framebuffer() { clear(); }
	public:
		void resize(GLuint _width, GLuint _height=0)
		{
			clear(); // call child clear func
			width = _width;
			height = (_height==0)?_width:_height;
			aspect = width/(float)height;
			create();
			resizeHook();
		}
		/* 오버라이딩하고 첫줄에 부모 가상함수를 꼭 호출해줘야함. */
		virtual void clear()
		{
			if( colorTex ) { glDeleteTextures(1, &colorTex); colorTex=0; }
			if( fbo ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
		}
		virtual void bind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glViewport(0, 0, width, height);
			glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			glClear(GL_COLOR_BUFFER_BIT);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);
		}
		/* for ms framebuffer */
		virtual GLuint getRenderedTex()
		{
			return colorTex;
		}
		virtual void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual void resizeHook() {}
	protected:
		virtual void createTexAndAttach()
		{
			// glTexStorage2D 텍스쳐 크기 고정
			// glTexImage2D 텍스쳐크기 변경가능 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glGenTextures(1, &colorTex);
			glBindTexture(GL_TEXTURE_2D, colorTex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
		}
	private:
		void create()
		{
			/* create FBO */
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			createTexAndAttach();

			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("color FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	};

	class TxFramebuffer: public Framebuffer
	{
	public:
		GLuint depth;
	public:
		TxFramebuffer(): Framebuffer(), depth(0) {}
		virtual ~TxFramebuffer() { clear(); }
	public:
		virtual void clear() final
		{
			Framebuffer::clear();
			if( depth ) { glDeleteRenderbuffers(1, &depth); depth=0; }
		}
		virtual void bind() final
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	protected:
		virtual void createTexAndAttach()
		{
			Framebuffer::createTexAndAttach();

			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		}
	};

	class RbFramebuffer: public Framebuffer
	{
	public:
		GLuint rbo;
	public:
		RbFramebuffer(): Framebuffer(), rbo(0) {}
		virtual ~RbFramebuffer() { clear(); }
	public:
		virtual void clear()
		{
			Framebuffer::clear();
			if( rbo ) { glDeleteRenderbuffers(1, &rbo); rbo=0; }
		}
		virtual void bind()
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	protected:
		virtual void createTexAndAttach()
		{
			Framebuffer::createTexAndAttach();

			// Create the depth buffer (stencil)
			// render buffer은 셰이더에서 접근할수없는 텍스쳐, 속도면에서 장점
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
		}
	};

	class MsFramebuffer: public RbFramebuffer
	{
	public:
		/* colorTex와 rbo를 multisampliing 모드로 생성 */
		const int samples = 8;
		Framebuffer intermediateFB;
	public:
		MsFramebuffer(): RbFramebuffer(), intermediateFB() {}
		virtual ~MsFramebuffer() { clear(); }
	public:
		virtual void clear() final
		{
			RbFramebuffer::clear();
			intermediateFB.clear();
		}
		virtual void bind() final
		{
			RbFramebuffer::bind();
			glEnable(GL_MULTISAMPLE);
		}
		virtual void unbind() final
		{
			/* msaa framebuffer은 일반 texture을 가진 frame버퍼에 blit 복사 해야함 */
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFB.fbo);

			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height
							  , GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual GLuint getRenderedTex() final
		{
			return intermediateFB.colorTex;
		}
		virtual void resizeHook()
		{
			intermediateFB.resize(width, height);
		}
	protected:
		virtual void createTexAndAttach()
		{
			/* multisampled FBO setting */
			glGenTextures(1, &colorTex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTex, 0);


			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);

			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
		}
	};
} // ! namespace lim

#endif