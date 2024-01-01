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
#include <glm/glm.hpp>

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
		GLint mag_filter = GL_LINEAR;
		GLint min_filter = GL_LINEAR_MIPMAP_LINEAR; // GL_NEAREST, LINEAR, *_MIPMAP_*
		glm::vec4 border_color = glm::vec4(0);
		GLint s_wrap_param = GL_REPEAT; // GL_CLAMP_TO_EDGE, BORDER(이후 border_color), GL_REPEAT , GL_MIRRORED_REPEAT
		GLint t_wrap_param = GL_REPEAT;
		GLint mipmap_max_level = 1000;
		GLenum src_format = GL_RGBA;
		GLenum src_chanel_type = GL_UNSIGNED_BYTE;

		// modified by initGL
		mutable GLuint tex_id=0;
		float aspect_ratio=1.f;

		// modified by initFromFile
		std::string file_path = "nopath/nofile.noformat";
		const char* file_format = file_path.c_str()+10;
		int nr_channels = 3;
		int bit_per_channel = 8;

	private:
	public:
		Texture();
		Texture(const Texture& src);
		Texture& operator=(const Texture& src);
		Texture(Texture&& src) noexcept;
		Texture& operator=(Texture&& src) noexcept;
		virtual ~Texture() noexcept;
	private:
		void* getDataAndPropsFromFile(std::string_view path);
	public:
		bool updateFormat(int nrChannels = 3, int bitPerChannel = 8, bool convertLinear = false, bool verbose = false);
		virtual void initGL(void* data = nullptr);
		bool initFromFile(std::string_view path, bool convertLinear = false);
		void deinitGL();
		GLuint getTexId() const;
	};

	class Texture3d: public Texture {
	public:
		int nr_depth=0;
		GLint r_wrap_param = GL_CLAMP_TO_EDGE;
		virtual void initGL(void* data = nullptr) override;
		void setDataWithDepth(int depth, void* data);
		Texture3d() = default;
		virtual ~Texture3d() noexcept = default;
		Texture3d(Texture3d&& src) noexcept;
		Texture3d& operator=(Texture3d&& src) noexcept;
	private:
		Texture3d(const Texture3d& src) = delete;
		Texture3d& operator=(const Texture3d& src) = delete;
		bool initFromFile(std::string_view path, bool convertLinear = false) = delete;
	};

	void drawTexToQuad(const GLuint texId, float gamma = 2.2f, float bias = 0.f, float gain = 1.f);
	void drawTex3dToQuad(const GLuint texId, float depth, float gamma = 2.2f, float bias = 0.f, float gain = 1.f);
	// for same size
	void copyTexToTex(const GLuint srcTexId, Texture& dstTex);
	// for diff size
	void copyTexToTex(const Texture& srcTex, Texture& dstTex);
}

#endif
