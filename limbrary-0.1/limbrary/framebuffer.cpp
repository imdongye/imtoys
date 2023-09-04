#include <limbrary/framebuffer.h>
#include <vector>
#include <limbrary/log.h>

namespace lim
{

	Framebuffer::Framebuffer(): clear_color({0.05f, 0.09f, 0.11f, 1.0f})
	{
		// todo
		fbo = color_tex = 0;
		width = height = 0;
		aspect = 0.f;
	}
	Framebuffer::~Framebuffer() 
	{
		if( fbo>0 ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
		if( color_tex>0 ) { glDeleteTextures(1, &color_tex); color_tex=0; }
	}
	void Framebuffer::genGlFboColor()
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
	}
	void Framebuffer::initGL()
	{
		Framebuffer::genGlFboColor();

		/* bind fbo */
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex, 0);

		std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, drawBuffers.data());

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( status !=GL_FRAMEBUFFER_COMPLETE )
			log::err("Framebuffer is not completed!! (%d)\n", status);

	}
	bool Framebuffer::resize(GLuint _width, GLuint _height)
	{
		if( _height==0 ) _height = _width;
		if( width==_width && height==_height )
			return false;
		width = _width; height = _height;
		aspect = width/(float)height;

		initGL();
		return true;
	}
	void Framebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
	}
	void Framebuffer::unbind() const
	{
		glEnable(GL_DEPTH_TEST);
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	/* ms framebuffer return intermidiate */
	GLuint Framebuffer::getRenderedTex() const
	{
		return color_tex;
	}

	
	TexFramebuffer::TexFramebuffer(): Framebuffer()
	{
		depth_tex = 0;
	}
	TexFramebuffer::~TexFramebuffer()
	{
		if( depth_tex>0 ) { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
	}
	void TexFramebuffer::genGLDepthTex()
	{
		if( depth_tex>0 ) { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
		glGenTextures(1, &depth_tex);
		glBindTexture(GL_TEXTURE_2D, depth_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	}
	void TexFramebuffer::initGL()
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
		if( status !=GL_FRAMEBUFFER_COMPLETE )
			log::err("TexFramebuffer is not completed!! (%d)\n", status);
	}
	void TexFramebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glDisable(GL_MULTISAMPLE);
		glEnable(GL_DEPTH_TEST);
	}


	RboFramebuffer::RboFramebuffer(): Framebuffer()
	{
		depth_rbo = 0;
	}
	RboFramebuffer::~RboFramebuffer()
	{
		if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
	}
	void RboFramebuffer::genGLDepthRbo()
	{
		if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
		glGenRenderbuffers(1, &depth_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
	}
	void RboFramebuffer::initGL()
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
		if( status !=GL_FRAMEBUFFER_COMPLETE )
			log::err("RboFramebuffer is not completed!! (%d)\n", status);
	}
	void RboFramebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
	}


	MsFramebuffer::MsFramebuffer(): RboFramebuffer(), intermediate_fb()
	{
	}
	MsFramebuffer::~MsFramebuffer()
	{
	}

	void MsFramebuffer::genGLFboMs()
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
	void MsFramebuffer::initGL()
	{
		intermediate_fb.resize(width, height);

		MsFramebuffer::genGLFboMs();

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, color_tex, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);

		std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, drawBuffers.data());

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if( status !=GL_FRAMEBUFFER_COMPLETE )
			log::err("MsFramebuffer is not completed!! (%d)\n", status);
	}
	void MsFramebuffer::bind() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	void MsFramebuffer::unbind() const
	{
		/* msaa framebuffer은 일반 texture을 가진 frame버퍼에 blit 복사 해야함 */
		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediate_fb.fbo);

		glDisable(GL_MULTISAMPLE);

		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height
							, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
	GLuint MsFramebuffer::getRenderedTex() const
	{
		return intermediate_fb.color_tex;
	}
}