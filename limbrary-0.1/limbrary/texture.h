//
//  2022-11-14 / im dong ye
// 	From: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
//
//	Todo:
//	1. ARB 확장이 뭐지 어셈블리?
//  2. GL_TEXTURE_MAX_ANISOTROPY_EXT
//	3. texture loading 분리
//

#ifndef __texture_h_
#define __texture_h_

#include <glad/glad.h>
#include <string>
#include <memory>

namespace lim
{
	struct TexBase
	{
	public:
		std::string path = "nopath/texture.png";
		const char* name = path.c_str()+5;
		const char* format = path.c_str()+10;

		int width=0, height=0;
		float aspect_ratio=1.f;
		int nr_channels = 3;

		GLuint tex_id=0;
		// 내부 저장 포맷, sRGB면 data에서 감마 변환
		GLint internal_format = GL_SRGB8_ALPHA8; // GL_R8, 
		GLenum src_format = GL_RGBA;
		GLenum src_chanel_type = GL_UNSIGNED_BYTE;
		int src_bit_per_channel = 8;

		GLint mag_filter = GL_LINEAR; // GL_NEAREST, LINEAR, *_MIPMAP_*
		GLint min_filter = GL_LINEAR_MIPMAP_LINEAR;
		GLint wrap_param = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_EDGE , GL_REPEAT , GL_REPEAT_MIRROR
		GLint mipmap_max_level = 1000;

	public:
		TexBase();
		virtual ~TexBase();
	public:
		void initGL(void* data = nullptr);
		void clear();
		void bind(GLuint pid, GLuint activeSlot, const std::string_view shaderUniformName) const;
	};

	TexBase* makeTextureFromImage(std::string_view path, GLint internalFormat);
	// nrChannels, bitPerChannel 0 is auto bit
	TexBase* makeTextureFromImageAuto(std::string_view path, bool convertLinear = false, int nrChannels = 0, int bitPerChannel = 0);

	
	/* do not use below for multisampling framebuffer */
	void Tex2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma=2.2f);
	void Gray2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma=2.2f);
}

#endif
