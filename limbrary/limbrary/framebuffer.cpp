#include <limbrary/framebuffer.h>
#include <vector>
#include <limbrary/tools/log.h>
#include <limbrary/application.h>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;



void IFramebuffer::resize(const ivec2& _size)
{
	if( size == _size ) {
		return;
	}
	size = _size;
	aspect = size.x/float(size.y);
	initGL();
	// glFinish(); Todo
	return;
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
void IFramebuffer::bind() const
{
	// prevState.capture();
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, size.x, size.y);
	glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	myBind();
	if(blendable) {
		// http://www.andersriggelsen.dk/glblendfunc.php
		// Todo: 같은모델에서 IOT 안됨
		glEnable(GL_BLEND);
		// glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		log::glError();
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
FramebufferNoDepth::~FramebufferNoDepth()
{
	deinitGL();
}
float* FramebufferNoDepth::makeFloatPixelsBuf() const {
	float* buf = new float[ size.x * size.y * color_tex.nr_channels ];
	GLuint format = color_tex.src_format;
	GLint readFbo;// backup
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadPixels(0,0, size.x, size.y, format, GL_FLOAT, buf);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
	return buf;
}

GLuint FramebufferNoDepth::getRenderedTexId() const 
{
	return color_tex.tex_id;
}
void FramebufferNoDepth::myInitGL() 
{
	color_tex.size = size;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);

	std::vector<GLenum> drawBuffers = {GL_COLOR_ATTACHMENT0};
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




FramebufferOnlyDepth::FramebufferOnlyDepth()
{
	depth_tex.internal_format = GL_DEPTH_COMPONENT32F;
	depth_tex.nr_channels = 1;
	depth_tex.src_chanel_type = GL_FLOAT;
	depth_tex.src_format = GL_DEPTH_COMPONENT;
	depth_tex.bit_per_channel = 32;
}
FramebufferOnlyDepth::~FramebufferOnlyDepth()
{
	deinitGL();
}
GLuint FramebufferOnlyDepth::getRenderedTexId() const
{
	return depth_tex.tex_id;
}
void FramebufferOnlyDepth::myInitGL() 
{
	depth_tex.size = size;
	depth_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex.tex_id, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
}
void FramebufferOnlyDepth::myDeinitGL() 
{
	depth_tex.deinitGL();
}
void FramebufferOnlyDepth::myBind() const
{
	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
}
void FramebufferOnlyDepth::myUnbind() const
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
FramebufferTexDepth::~FramebufferTexDepth()
{
	deinitGL();
}
GLuint FramebufferTexDepth::getRenderedTexId() const
{
	return color_tex.tex_id;
}
void FramebufferTexDepth::myInitGL()
{
	color_tex.size = size;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);

	depth_tex.size = size;
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
FramebufferRbDepth::~FramebufferRbDepth()
{
	deinitGL();
}
GLuint FramebufferRbDepth::getRenderedTexId() const
{
	return color_tex.tex_id;
}
void FramebufferRbDepth::myInitGL()
{
	color_tex.size = size;
	color_tex.initGL();
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_tex.tex_id, 0);
	
	assert( depth_rbo_id==0 ); // myDeinitGL이 먼저 호출돼서 0이여야함.
	glGenRenderbuffers(1, &depth_rbo_id);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_id);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x, size.y);
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
	: IFramebuffer(), intermediate_fb(nrChannels, bitPerChannel)
{
	int maxMsSamples;
	glGetIntegerv(GL_MAX_SAMPLES, &maxMsSamples);
	samples = glm::min(maxMsSamples,_samples);
}
FramebufferMs::~FramebufferMs()
{
	deinitGL();
}
GLuint FramebufferMs::getRenderedTexId() const
{
	return intermediate_fb.color_tex.tex_id;
}
void FramebufferMs::myInitGL()
{
	GLenum interFormat = intermediate_fb.color_tex.internal_format;
	if( ms_color_tex_id>0 ) { glDeleteTextures(1, &ms_color_tex_id); ms_color_tex_id=0; }
	glGenTextures(1, &ms_color_tex_id);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ms_color_tex_id);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, interFormat, size.x, size.y, GL_TRUE);

	if( ms_depth_rbo_id>0 ) { glDeleteRenderbuffers(1, &ms_depth_rbo_id); ms_depth_rbo_id=0; }
	glGenRenderbuffers(1, &ms_depth_rbo_id);
	glBindRenderbuffer(GL_RENDERBUFFER, ms_depth_rbo_id);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, size.x, size.y);

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

	intermediate_fb.resize(size);
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

	glBlitFramebuffer(0, 0, size.x, size.y, 0, 0, size.x, size.y
						, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}