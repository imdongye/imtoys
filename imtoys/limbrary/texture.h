//
//  2022-11-14 / im dong ye
//
//	Todo:
//	1. ARB 확장이 뭐지 어셈블리?
//  2. GL_TEXTURE_MAX_ANISOTROPY_EXT
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include "limclude.h"

namespace lim
{
	struct Texture
	{
	public:
		GLuint tex_id=0;
		int width=0, height=0;
		int nr_channels=0;
		std::string tag;
		std::string path;
		// for simp mesh export
		const char* internal_model_path = nullptr;
		const char* format;
		// 내부 저장 포맷, sRGB면 감마 변환
		GLint internal_format; 
		GLenum src_format, src_chanel_type;
		int bit_per_channel;
	private:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
	public:
		/* load texture */
		Texture(const std::string_view _path, GLint internalFormat=GL_RGB32F) // GL_RGB8, GL_SRGB8
			: path(_path), internal_format(internalFormat), format(path.c_str()+path.rfind('.')+1)
		{
			void* data;

			if( stbi_is_hdr(path.c_str()) ) {
				data=stbi_loadf(path.c_str(), &width, &height, &nr_channels, 0);
				if( stbi_is_16_bit(path.c_str()) ) {
					src_chanel_type = GL_HALF_FLOAT;
					bit_per_channel = 16;
				} else {
					src_chanel_type = GL_FLOAT;
					bit_per_channel = 32;
				}
			} 
			else {
				data=stbi_load(path.c_str(), &width, &height, &nr_channels, 0);
				src_chanel_type = GL_UNSIGNED_BYTE;
				bit_per_channel = 8;
			}
			if( !data ) {
				Logger::get(1).log("texture failed to load at path: %s\n", path.c_str());
				return;
			}

			src_format = GL_RGBA;
			switch( nr_channels ) {
			case 1: src_format = GL_RED; break;
			case 2: src_format = GL_RG; break;
			case 3: src_format = GL_RGB; break;
			case 4: src_format = GL_RGBA; break;
			}

			
			glGenTextures(1, &tex_id);
			glBindTexture(GL_TEXTURE_2D, tex_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// GL_LINEAR_MIPMAP_LINEAR : 두개의 side mipmap에서 보간에 보간한다.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);


			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_chanel_type, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);


			printInfo();

			stbi_image_free(data);
		}
		~Texture()
		{
			printf("texture clear\n");
			clear();
		}
		void printInfo()
		{
			printf("texID:%d, %dx%d, nr_ch:%d, bit:%d, fm:%s, aspect:%f\n", tex_id, width, height, nr_channels, bit_per_channel, format, width/(float)height);
		}
		std::shared_ptr<Texture> clone()
		{
			return std::make_shared<Texture>(path, internal_format);
		}
		void reload(const std::string_view _path, GLint internalFormat=GL_RGB32F)
		{
			clear();
			Texture(_path, internalFormat);
		}
		void bind(GLuint activeSlot, const std::string_view shaderUniformName) const
		{
			glActiveTexture(GL_TEXTURE0 + activeSlot);
			glBindTexture(GL_TEXTURE_2D, tex_id);
			if( shaderUniformName.length()>0 ) {
				GLint pid;
				glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
				GLint loc = glGetUniformLocation(pid, shaderUniformName.data());
				glUniform1i(loc, activeSlot);
			}
		}
	private:
		void clear()
		{
			if( glIsTexture(tex_id) ) {
				glDeleteTextures(1, &tex_id);
				tex_id=0;
			}
		}
	};

	static void textureToFBO(GLuint tex_id, GLsizei width, GLsizei height, GLuint fbo=0, float gamma=2.2f)
	{
		static Program toQuadProg = Program("toQuad");
		static GLuint quadVAO = 0;

		if( glIsVertexArray(quadVAO)==GL_TRUE ) {
			// Array for full-screen quad
			GLfloat verts[] ={
				-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
			};

			// Set up the buffers
			GLuint vbo;
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(GLfloat), verts, GL_STATIC_DRAW);

			// Set up the vertex array object
			glGenVertexArrays(1, &quadVAO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(0);  // Vertex position

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}
		if( glIsProgram(toQuadProg.pid)==GL_TRUE ) {
			toQuadProg.attatch("tex_to_quad.vs").attatch("tex_to_quad.fs").link();
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);
		glDisable(GL_FRAMEBUFFER_SRGB);

		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		toQuadProg.use();

		setUniform(toQuadProg.pid, "tex", 0);

		setUniform(toQuadProg.pid, "gamma", gamma);

		glBindTexture(GL_TEXTURE_2D, tex_id);
		glActiveTexture(GL_TEXTURE0);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	static void textureToFramebuffer(GLuint texID, Framebuffer const *fb, float gamma=2.2f)
	{
		textureToFBO(texID, fb->fbo, fb->width, fb->height, gamma);
	}

	inline glm::vec3 sample(void* buf, int w, int h, int x, int y, int n, int isHdr=0)
	{
		if( isHdr ) {
			float* data = (float*)buf;
			return glm::vec3(data[(x+y*w)*n], data[(x+y*w)*n+1], data[(x+y*w)*n+2]);
		} else {
			unsigned char* data = (unsigned char*)buf;
			return glm::vec3(data[(x+y*w)*n], data[(x+y*w)*n+1], data[(x+y*w)*n+2]);
		}
	}
	bool isNormal(const glm::vec3 v)
	{
		glm::vec3 n = (v-glm::vec3(127))/127.f;
		float l = length(n);
		return l>0.9 && l<1.1;
	}
}

#endif
