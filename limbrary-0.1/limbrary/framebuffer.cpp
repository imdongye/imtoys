#include <limbrary/framebuffer.h>
#include <vector>
#include <limbrary/log.h>

namespace lim
{
	Framebuffer::Framebuffer()
	{
	}
	Framebuffer::Framebuffer(Framebuffer&& src) noexcept
	{
		fbo = src.fbo;
		color_tex = src.color_tex;
		src.fbo = 0;
		src.color_tex = 0;
		
		clear_color = src.clear_color;
		width = src.width;
		height = src.height;
		aspect = src.aspect;
	}
	Framebuffer& Framebuffer::operator=(Framebuffer&& src) noexcept
	{
		if( this==&src )
			return *this;
		Framebuffer::~Framebuffer();

		fbo = src.fbo;
		color_tex = src.color_tex;
		src.fbo = 0;
		src.color_tex = 0;

		clear_color = src.clear_color;
		width = src.width;
		height = src.height;
		aspect = src.aspect;
		return *this;
	}
	Framebuffer::~Framebuffer() noexcept
	{
		if( fbo>0 ) 	  { glDeleteFramebuffers(1, &fbo); fbo=0; }
		if( color_tex>0 ) { glDeleteTextures(1, &color_tex); color_tex=0; }
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
	bool Framebuffer::resize(GLuint _width, GLuint _height)
	{
		if( width==_width && height==_height )
			return false;
		width = _width; height = _height;
		aspect = width/(float)height;
		initGL();
		return true;
	}
	bool Framebuffer::resize(GLuint square)
	{
		return resize(square, square);
	}
	void Framebuffer::bind() const
	{
		glDisable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	void Framebuffer::unbind() const
	{
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	TexFramebuffer::TexFramebuffer(TexFramebuffer&& src) noexcept
		: Framebuffer(std::move(src))
	{
		depth_tex = src.depth_tex;
		src.depth_tex = 0;
	}
	TexFramebuffer& TexFramebuffer::operator=(TexFramebuffer&& src) noexcept
	{
		if( this==&src )
			return *this;
		TexFramebuffer::~TexFramebuffer();
		Framebuffer::operator=(std::move(src));

		depth_tex = src.depth_tex;
		src.depth_tex = 0;
		return *this;
	}
	TexFramebuffer::~TexFramebuffer() noexcept
	{
		if( depth_tex>0 ) { glDeleteTextures(1, &depth_tex); depth_tex = 0; }
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
	RboFramebuffer::RboFramebuffer(RboFramebuffer&& src) noexcept
		: Framebuffer(std::move(src))
	{
		depth_rbo = src.depth_rbo;
		src.depth_rbo = 0;
	}
	RboFramebuffer& RboFramebuffer::operator=(RboFramebuffer&& src) noexcept
	{
		if( this==&src )
			return *this;
		RboFramebuffer::~RboFramebuffer();
		Framebuffer::operator=(std::move(src));

		depth_rbo = src.depth_rbo;
		src.depth_rbo = 0;
		return *this;
	}
	RboFramebuffer::~RboFramebuffer() noexcept
	{
		if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
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
	void RboFramebuffer::genGLDepthRbo()
	{
		if( depth_rbo>0 ) { glDeleteRenderbuffers(1, &depth_rbo); depth_rbo=0; }
		glGenRenderbuffers(1, &depth_rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
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


	MsFramebuffer::MsFramebuffer(int _samples): RboFramebuffer(), intermediate_fb()
	{
		samples = _samples;
	}
	MsFramebuffer::MsFramebuffer(MsFramebuffer&& src) noexcept
	{
		intermediate_fb = std::move(src.intermediate_fb);
	}
	MsFramebuffer& MsFramebuffer::operator=(MsFramebuffer&& src) noexcept
	{
		if( this==&src )
			return *this;
		RboFramebuffer::operator=(std::move(src));

		intermediate_fb = std::move(src.intermediate_fb);
		return *this;
	}
	MsFramebuffer::~MsFramebuffer() noexcept
	{
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
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	GLuint MsFramebuffer::getRenderedTex() const
	{
		return intermediate_fb.color_tex;
	}
}