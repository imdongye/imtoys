//
//  2022-11-14 / im dong ye
//	edit HDRView by pf Hyun Joon Shin
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

		// 내부 저장 포맷, sRGB면 data에서 감마 변환
		GLint internal_format;
		GLenum src_format, src_chanel_type;
		int src_bit_per_channel;

		GLuint tex_id=0;
		int width=0, height=0;
		float aspect_ratio=1.f;

		// GL_NEAREST, LINEAR, *_MIPMAP_*
		GLint mag_filter = GL_LINEAR;
		// GL_CLAMP_TO_EDGE , GL_REPEAT , GL_REPEAT_MIRROR
		GLint min_filter = GL_LINEAR_MIPMAP_LINEAR;
		GLint wrap_param = GL_CLAMP_TO_EDGE;
		GLint mipmap_max_level = 1000;

		int nr_channels=0;
	public:
		TexBase(GLint internalFormat = GL_SRGB8);
		virtual ~TexBase();
	public:
		void create(int _width, int _height, GLint internalFormat, GLuint srcFormat, GLuint srcChanelType, void* data);
		void create(void* data = nullptr);
		void clear();
		void bind(GLuint pid, GLuint activeSlot, const std::string_view shaderUniformName) const;
	};

	bool loadImageToTex(std::string_view path, TexBase& tex);
	
	/* do not use below for multisampling framebuffer */
	void Tex2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma=2.2f);
	void Gray2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma=2.2f);
}

#endif
