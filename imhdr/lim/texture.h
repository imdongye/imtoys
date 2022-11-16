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
		GLuint texID=0;
		int width=0, height=0;
		int nr_channels=0;
		std::string path;
		const char* format;
		// 내부 저장 포맷, sRGB면 감마 변환
		GLint internal_format; 
		GLenum src_format, src_chanel_type;
		int bit_per_channel;
	public:
		// GL_RGB8, GL_SRGB8
		Texture(const std::string_view _path, GLint internalFormat=GL_RGB32F)
			: path(_path), internal_format(internalFormat)
		{
			void* data;
			format = path.c_str()+path.rfind('.')+1;

			if( stbi_is_hdr(path.c_str()) ) {
				data=stbi_loadf(path.c_str(), &width, &height, &nr_channels, 0);
				src_chanel_type = ( stbi_is_16_bit(path.c_str()) ) ? GL_HALF_FLOAT : GL_FLOAT;
				bit_per_channel = (stbi_is_16_bit(path.c_str())) ? 16 : 32;
			} else {
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
			case 1: src_format = GL_R8; break;
			case 2: src_format = GL_RG; break;
			case 3: src_format = GL_RGB; break;
			case 4: src_format = GL_RGBA; break;
			}

			initOpenGL(data);
			printInfo();

			stbi_image_free(data);
		}
		~Texture()
		{
			clear();
		}
		void printInfo()
		{
			printf("texID:%d, %dx%d, nr_ch:%d, bit:%d, fm:%s\n", texID, width, height, nr_channels, bit_per_channel, format);
		}
		void clear()
		{
			if( texID ) {
				glDeleteTextures(1, &texID);
				texID=0;
			}
		}
		void bind(GLuint activeSlot, const std::string_view shaderUniformName) const
		{
			glActiveTexture(GL_TEXTURE0 + activeSlot);
			glBindTexture(GL_TEXTURE_2D, texID);
			if( shaderUniformName.length()>0 ) {
				GLint pid;
				glGetIntegerv(GL_CURRENT_PROGRAM, &pid);
				GLint loc = glGetUniformLocation(pid, shaderUniformName.data());
				glUniform1i(loc, activeSlot);
			}
		}

	private:
		void initOpenGL(void* data)
		{
			if( !data ) return;

			if( texID>0 ) clear();

			glGenTextures(1, &texID);
			glBindTexture(GL_TEXTURE_2D, texID);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// GL_LINEAR_MIPMAP_LINEAR : 두개의 side mipmap에서 보간에 보간한다.
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

			/*glTexture... 은 texID로 지정, gl4.5부터 사용가능
			* glCreateTexture은 bind 바로됨, gl4.5부터 사용가능
			* glStorate2D는 데이터복사는 따로해줘야되고 Image2D와 다르게 크기나 포맷 변경안됨
			*/
			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_chanel_type, data);
			//glTexStorage2D(GL_TEXTURE_2D, 1, internal_format, width, height); // 4.2
			//glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};


	/* c++17 global data member can initialize in declaration with inline keyword */
	inline static Program toScrProgram = Program("toScr");
	inline static GLuint quadVAO = 0;
	static void textureToBackBuf(GLuint texID, GLsizei width, GLsizei height, bool toSRGB = false)
	{
		if( toScrProgram.ID==0 ) {
			toScrProgram.attatch("fb_to_scr.vs").attatch("fb_to_scr.fs").link();
			GLuint loc = glGetUniformLocation(toScrProgram.use(), "screenTex");
			glUniform1i(loc, 0);
		}
		if( quadVAO == 0 ) {
			// Array for full-screen quad
			GLfloat verts[] ={
				-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
				-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
			};
			GLfloat tc[] ={
				0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
				0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
			};

			// Set up the buffers
			unsigned int handle[2];
			glGenBuffers(2, handle);
			glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
			glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
			glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

			// Set up the vertex array object
			glGenVertexArrays(1, &quadVAO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
			glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(0);  // Vertex position
			glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
			glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(1);  // Texture coordinates
			glBindVertexArray(0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		if( toSRGB ) glEnable(GL_FRAMEBUFFER_SRGB);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		toScrProgram.use();
		glBindVertexArray(quadVAO);
		// use the color attachment texture as the texture of the quad plane
		glBindTexture(GL_TEXTURE_2D, texID);
		glActiveTexture(GL_TEXTURE0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
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
