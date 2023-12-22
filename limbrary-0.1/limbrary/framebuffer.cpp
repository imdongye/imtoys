#include <limbrary/framebuffer.h>
#include <vector>
#include <limbrary/log.h>

#include <limbrary/asset_lib.h>

using namespace lim;


IFramebuffer::IFramebuffer(IFramebuffer&& src) noexcept {
	*this = std::move(src);
}
IFramebuffer& IFramebuffer::operator=(IFramebuffer&& src) noexcept {
	if( this!=&src ) {
		deinitGL();

		fbo = src.fbo;
		src.fbo = 0;

		width = src.width;
		height = src.height;
		aspect = src.aspect;
		clear_color = src.clear_color;
	}
	return *this;
}
void IFramebuffer::initGL()
{
	deinitGL();

	/* bind fbo */
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	myInitGL();

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( status !=GL_FRAMEBUFFER_COMPLETE )
		log::err("Framebuffer is not completed!! (%d)\n", status);

}
void IFramebuffer::deinitGL()
{
	if( fbo>0 ) { glDeleteFramebuffers(1, &fbo); fbo=0; }
	myDeinitGL();
}
bool IFramebuffer::resize(GLuint _width, GLuint _height)
{
	_height = (_height<0)?_width:_height;
	if( width==_width && height==_height )
		return false;
	width = _width; height = _height;
	aspect = width/(float)height;
	initGL();
	glFlush();
	glFinish();
	return true;
}
void IFramebuffer::bind() const
{
	// prevState.capture();
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);
	glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	myBind();
	if(blendable) {
		// http://www.andersriggelsen.dk/glblendfunc.php
		// Todo: 같은모델에서 IOT 안됨
		glEnable(GL_BLEND);
		// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		utils::glErr("asdf");
	}
}
void IFramebuffer::unbind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	myUnbind();
	// prevState.restore();
}



FramebufferNoDepth::FramebufferNoDepth(int nrChannels, int bitPerChannel)
{
	color_tex.updateFormat(nrChannels, bitPerChannel);
}
FramebufferNoDepth::FramebufferNoDepth(FramebufferNoDepth&& src) noexcept
	: IFramebuffer(std::move(src))
{
	color_tex = std::move(src.color_tex);
}
FramebufferNoDepth& FramebufferNoDepth::operator=(FramebufferNoDepth&& src) noexcept
{
	if( this!=&src ) {
		IFramebuffer::operator=(std::move(src));

		color_tex = std::move(src.color_tex);
	}
	return *this;
}
FramebufferNoDepth::~FramebufferNoDepth() noexcept
{
	deinitGL();
}
/* override albe */
GLuint FramebufferNoDepth::getRenderedTex() const
{
	return color_tex.tex_id;
}
void FramebufferNoDepth::myInitGL() 
{
	color_tex.width = width;
	color_tex.height = height;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);

	std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers.data());
}
void FramebufferNoDepth::myDeinitGL() 
{
	color_tex.deinitGL();
}
void FramebufferNoDepth::myBind() const
{
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
}
void FramebufferNoDepth::myUnbind() const
{
}



FramebufferTexDepth::FramebufferTexDepth(int nrChannels, int bitPerChannel)
{
	color_tex.updateFormat(nrChannels, bitPerChannel);

	// GL_DEPTH_COMPONENT32F overkill?
	// From: https://gamedev.stackexchange.com/questions/111955/gl-depth-component-vs-gl-depth-component32
	depth_tex.internal_format = GL_DEPTH_COMPONENT32F;
	depth_tex.nr_channels = 1;
	depth_tex.src_chanel_type = GL_FLOAT;
	depth_tex.src_format = GL_DEPTH_COMPONENT;
	depth_tex.bit_per_channel = 32;
}
FramebufferTexDepth::FramebufferTexDepth(FramebufferTexDepth&& src) noexcept
	: IFramebuffer(std::move(src))
{
	color_tex = std::move(src.color_tex);
	depth_tex = std::move(src.depth_tex);
}
FramebufferTexDepth& FramebufferTexDepth::operator=(FramebufferTexDepth&& src) noexcept
{
	if( this!=&src ) {
		IFramebuffer::operator=(std::move(src));

		color_tex = std::move(src.color_tex);
		depth_tex = std::move(src.depth_tex);
	}
	return *this;
}
FramebufferTexDepth::~FramebufferTexDepth() noexcept
{
	deinitGL();
}
/* override albe */
GLuint FramebufferTexDepth::getRenderedTex() const
{
	return color_tex.tex_id;
}
void FramebufferTexDepth::myInitGL()
{
	color_tex.width = width;
	color_tex.height = height;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);

	depth_tex.width = width;
	depth_tex.height = height;
	depth_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.tex_id, 0);

	std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers.data());
}
void FramebufferTexDepth::myDeinitGL()
{
	color_tex.deinitGL();
	depth_tex.deinitGL();
}
void FramebufferTexDepth::myBind() const
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}
void FramebufferTexDepth::myUnbind() const
{
}


FramebufferRbDepth::FramebufferRbDepth(int nrChannels, int bitPerChannel)
{
	color_tex.updateFormat(nrChannels, bitPerChannel);
}
FramebufferRbDepth::FramebufferRbDepth(FramebufferRbDepth&& src) noexcept
	: IFramebuffer(std::move(src))
{
	color_tex = std::move(src.color_tex);
	depth_rbo_id = src.depth_rbo_id;
	src.depth_rbo_id = 0;
}
FramebufferRbDepth& FramebufferRbDepth::operator=(FramebufferRbDepth&& src) noexcept
{
	if( this!=&src ) {
		IFramebuffer::operator=(std::move(src));

		color_tex = std::move(src.color_tex);
		depth_rbo_id = src.depth_rbo_id;
		src.depth_rbo_id = 0;
	}
	return *this;
}
FramebufferRbDepth::~FramebufferRbDepth() noexcept
{
	deinitGL();
}
/* override albe */
GLuint FramebufferRbDepth::getRenderedTex() const
{
	return color_tex.tex_id;
}
void FramebufferRbDepth::myInitGL()
{
	color_tex.width = width;
	color_tex.height = height;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);
	
	if( depth_rbo_id>0 ) { log::err("myDeinitGL이 먼저 호출돼서 절대 들어올수 없음\n"); }
	glGenRenderbuffers(1, &depth_rbo_id);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	// GL_DEPTH_ATTACHMENT32F 나중에 문제될수있음. 테스트
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_rbo_id);

	std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers.data());
}
void FramebufferRbDepth::myDeinitGL()
{
	color_tex.deinitGL();
	if( depth_rbo_id>0 ) { glDeleteRenderbuffers(1, &depth_rbo_id); depth_rbo_id=0; }
}
void FramebufferRbDepth::myBind() const
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}
void FramebufferRbDepth::myUnbind() const
{
}


FramebufferMs::FramebufferMs(int _samples, int nrChannels, int bitPerChannel)
	: IFramebuffer(), samples(glm::min(utils::getMsMaxSamples(),_samples))
	, intermediate_fb(nrChannels, bitPerChannel)
{
}
FramebufferMs::FramebufferMs(FramebufferMs&& src) noexcept
	: IFramebuffer(std::move(src))
{
	samples = src.samples;
	intermediate_fb = std::move(src.intermediate_fb);
	ms_color_tex_id = src.ms_color_tex_id;
	ms_depth_rbo_id = src.ms_depth_rbo_id;
	src.ms_color_tex_id = 0;
	src.ms_depth_rbo_id = 0;
}
FramebufferMs& FramebufferMs::operator=(FramebufferMs&& src) noexcept
{
	if( this!=&src ) {
		IFramebuffer::operator=(std::move(src));

		samples = src.samples;
		intermediate_fb = std::move(src.intermediate_fb);
		ms_color_tex_id = src.ms_color_tex_id;
		ms_depth_rbo_id = src.ms_depth_rbo_id;
		src.ms_color_tex_id = 0;
		src.ms_depth_rbo_id = 0;
	}
	return *this;
}
FramebufferMs::~FramebufferMs() noexcept
{
	deinitGL();
}
/* override able */
GLuint FramebufferMs::getRenderedTex() const
{
	return intermediate_fb.color_tex.tex_id;
}
void FramebufferMs::myInitGL()
{
	GLenum interFormat = intermediate_fb.color_tex.internal_format;
	if( ms_color_tex_id>0 ) { glDeleteTextures(1, &ms_color_tex_id); ms_color_tex_id=0; }
	glGenTextures(1, &ms_color_tex_id);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex_id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, interFormat, width, height, GL_TRUE);

	if( ms_depth_rbo_id>0 ) { glDeleteRenderbuffers(1, &ms_depth_rbo_id); ms_depth_rbo_id=0; }
	glGenRenderbuffers(1, &ms_depth_rbo_id);
	glBindRenderbuffer(GL_RENDERBUFFER, ms_depth_rbo_id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex_id, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ms_depth_rbo_id);

	std::vector<GLenum> drawBuffers ={GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers.data());

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if( status != GL_FRAMEBUFFER_COMPLETE )
		log::err("MsFramebuffer is not completed!! (%d)\n", status);

	intermediate_fb.resize(width, height);
}
void FramebufferMs::myDeinitGL()
{
	if( ms_depth_rbo_id>0 ) { glDeleteRenderbuffers(1, &ms_depth_rbo_id); ms_depth_rbo_id=0; }
	if( ms_color_tex_id>0 ) { glDeleteTextures(1, &ms_color_tex_id); ms_color_tex_id=0; }
	intermediate_fb.deinitGL();
}
void FramebufferMs::myBind() const
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}
void FramebufferMs::myUnbind() const
{
	/* msaa framebuffer은 일반 texture을 가진 frame버퍼에 blit 복사 해야함 */
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediate_fb.fbo);

	glDisable(GL_MULTISAMPLE);

	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height
						, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}