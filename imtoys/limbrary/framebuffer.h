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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

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
		Framebuffer(): clear_color({0.2f, 0.3f, 0.3f, 1.0f})
		{
			fbo = color_tex = 0;
			aspect= width = height = 0;
		}
		virtual ~Framebuffer() 
		{
			if( fbo>0 ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
			if( color_tex>0 ) { glDeleteTextures(1, &color_tex); color_tex=0; }
		}
	protected:
		void genGlFboColor()
		{
			if( fbo>0 ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
			glGenFramebuffers(1, &fbo);

			if( color_tex>0 ) { glDeleteTextures(1, &color_tex); color_tex=0; }
			glGenTextures(1, &color_tex);
			glBindTexture(GL_TEXTURE_2D, color_tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		virtual void initGL()
		{
			Framebuffer::genGlFboColor();

			/* bind fbo */
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);

			std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
			glDrawBuffers(1, drawBuffers.data());

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status !=GL_FRAMEBUFFER_COMPLETE ) std::cerr<<"Framebuffer is not completed!! ("<<status<<")"<<std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	public:
		bool setSize(GLuint _width, GLuint _height=0)
		{
			if( _height==0 ) _height = _width;
			if( width==_width && height==_height )
				return false;
			width = _width; height = _height;
			aspect = width/(float)height;

			initGL();
			return true;
		}
		virtual void bind() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glViewport(0, 0, width, height);
			glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
			glClear(GL_COLOR_BUFFER_BIT);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);
		}
		virtual void unbind() const
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		/* ms framebuffer return intermidiate */
		virtual GLuint getRenderedTex() const
		{
			return color_tex;
		}
	};

	class TexFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_tex;
	public:
		TexFramebuffer(): Framebuffer()
		{
			depth_tex = 0;
		}
		virtual ~TexFramebuffer() override
		{
			if( depth_tex>0 ) { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
		}
	protected:
		void genGLDepthTex()
		{
			if( depth_tex>0 ) { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
			glGenTextures(1, &depth_tex);
			glBindTexture(GL_TEXTURE_2D, depth_tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		virtual void initGL() override
		{
			Framebuffer::genGlFboColor();
			TexFramebuffer::genGLDepthTex();

			/* bind fbo */
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex, 0);

			std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
			glDrawBuffers(1, drawBuffers.data());

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status !=GL_FRAMEBUFFER_COMPLETE ) std::cerr<<"TexFramebuffer is not completed!! ("<<status<<")"<<std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	public:
		virtual void bind() const override
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	};

	class RboFramebuffer: public Framebuffer
	{
	public:
		GLuint depth_rbo;
	public:
		RboFramebuffer(): Framebuffer()
		{
			depth_rbo = 0;
		}
		virtual ~RboFramebuffer()
		{
			if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
		}
	protected:
		void genGLDepthRbo()
		{
			if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
			glGenRenderbuffers(1, &depth_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
		}
		virtual void initGL() override
		{
			Framebuffer::genGlFboColor();
			RboFramebuffer::genGLDepthRbo();

			/* bind fbo */
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

			std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
			glDrawBuffers(1, drawBuffers.data());

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status !=GL_FRAMEBUFFER_COMPLETE ) std::cerr<<"RboFramebuffer is not completed!! ("<<status<<")"<<std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	public:
		virtual void bind() const override
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	};

	class MsFramebuffer: public RboFramebuffer
	{
	public:
		/* colorTex와 rbo를 multisampliing 모드로 생성 */
		const int samples = 8;
		Framebuffer intermediate_fb;
	public:
		MsFramebuffer(): RboFramebuffer(), intermediate_fb()
		{
		}
		virtual ~MsFramebuffer() override
		{
		}
	protected:
		void genGLFboMs()
		{
			if( fbo>0 ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
			glGenFramebuffers(1, &fbo);

			if( color_tex>0 ) { glDeleteTextures(1, &color_tex); color_tex=0; }
			glGenTextures(1, &color_tex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, color_tex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

			if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
			glGenRenderbuffers(1, &depth_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
		virtual void initGL() override
		{
			intermediate_fb.setSize(width, height);

			MsFramebuffer::genGLFboMs();

			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, color_tex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

			std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
			glDrawBuffers(1, drawBuffers.data());

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if( status !=GL_FRAMEBUFFER_COMPLETE ) std::cerr<<"MsFramebuffer is not completed!! ("<<status<<")"<<std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	public:
		virtual void bind() const override
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_MULTISAMPLE);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
		virtual void unbind() const override
		{
			/* msaa framebuffer은 일반 texture을 가진 frame버퍼에 blit 복사 해야함 */
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediate_fb.fbo);

			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height
							  , GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual GLuint getRenderedTex() const override
		{
			return intermediate_fb.color_tex;
		}
	};
}

#endif