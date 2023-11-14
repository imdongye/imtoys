/*
	2022-11-14 / im dong ye
	From: https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml

	Note:
	opengl srgb변환은 8bit 이미지에만 적용할수있음.

	Todo:
	1. ARB 확장이 뭐지 어셈블리?
	2. GL_TEXTURE_MAX_ANISOTROPY_EXT
*/

#ifndef __texture_h_
#define __texture_h_

#include <glad/glad.h>
#include <string>
#include <memory>

namespace lim
{
	// clone able
	class Texture
	{
	public:
		// modified by user
		std::string name = "nonamed texture";
		int width=0, height=0; // initFromFile을 사용하면 수정됨.
		GLint internal_format = GL_RGB8; // GL_R8, GL_SRGB8_ALPHA8(internal gamma collection)
		GLint mag_filter = GL_LINEAR; // GL_NEAREST, LINEAR, *_MIPMAP_*
		GLint min_filter = GL_LINEAR_MIPMAP_LINEAR;
		GLint wrap_param = GL_CLAMP_TO_EDGE; // GL_CLAMP_TO_EDGE , GL_REPEAT , GL_REPEAT_MIRROR
		GLint mipmap_max_level = 1000;
		GLenum src_format = GL_RGBA;
		GLenum src_chanel_type = GL_UNSIGNED_BYTE;

		// modified by initGL
		GLuint tex_id=0;
		float aspect_ratio=1.f;

		// modified by initFromFile
		std::string file_path = "nopath/nofile.noformat";
		const char* file_format = file_path.c_str()+10;
		int nr_channels = 3;
		int bit_per_channel = 8;

	private:
		Texture& operator=(const Texture&) = delete;
	public:
		Texture();
		Texture(const Texture& src);
		Texture(Texture&& src) noexcept;
		Texture& operator=(Texture&& src) noexcept;
		virtual ~Texture() noexcept;
	private:
		void* getDataAndPropsFromFile(std::string_view path);
	public:
		bool updateFormat(int nrChannels = 3, int bitPerChannel = 8, bool convertLinear = false);
		void initGL(void* data = nullptr);
		bool initFromFile(std::string_view path, bool convertLinear = false);
		void deinitGL();
	};

	void drawTexToQuad(const GLuint texId, float gamma = 2.2f);
	// for same size
	void copyTexToTex(const GLuint srcTexId, Texture& dstTex);
	// for diff size
	void copyTexToTex(const Texture& srcTex, Texture& dstTex);
}

#endif
