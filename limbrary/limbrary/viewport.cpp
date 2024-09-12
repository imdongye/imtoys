#include <limbrary/viewport.h>
#include <limbrary/application.h>
#include <limbrary/tools/text.h>

using namespace lim;


Viewport::Viewport(IFramebuffer* createdFB, const char* _name)
{
	assert(createdFB);
	name = fmtStrToBuf("%s##%s", _name, AppBase::g_app_name);
	own_framebuffer = createdFB;
	own_framebuffer->resize(256, 256); // default size
}
void Viewport::resize(GLuint _width, GLuint _height)
{
	own_framebuffer->resize(_width, _height);
	for( auto& cb : resize_callbacks ) {
		cb(_width, _height);
	}
}