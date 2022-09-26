//
//  2022-07-20 / im dong ye
//  edit learnopengl code
// 
//	Todo:
//	1. 10bit hdr지원
//	2. bumpmap등은 linear space로 읽어서 linear space로 출력함
//

#ifndef TEXTURE_H
#define TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace lim
{
	struct Texture
	{
		GLuint id;
		std::string type;
		std::string path; // relative path+filename or only filename
	};

	static inline void setTexParam(GLuint minFilter = GL_LINEAR, GLuint warp = GL_CLAMP_TO_EDGE)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// GL_LINEAR_MIPMAP_LINEAR : mipmap에서 찾아서 4점을 보간하고 다른 mipmap에서 찾아서 또 섞는다.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, warp);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, warp);
	}

	static GLuint loadTextureFromFile(std::string_view path, bool toLinear = true)
	{
		const char* cpath = path.data();

		GLuint texID=0;
		glGenTextures(1, &texID);

		int w, h, channels;
		// 0 => comp 있는대로
		void* buf = stbi_load(cpath, &w, &h, &channels, 4); // todo: 0

		if( !buf ) {
			Logger::get().log("texture failed to load at path: %s\n", cpath);
			stbi_image_free(buf);
			return texID;
		}
		else {
			Logger::get().log("texture loaded : %s , %dx%d, %d channels\n", cpath, w, h, channels);
		}

		// load into vram
		GLenum format = GL_RGBA;
		switch( channels ) {
		case 1: format = GL_ALPHA; break;
		case 2: format = 0; break;
		case 3:
			if( toLinear ) format = GL_SRGB8;
			else format = GL_RGB; // if hdr => 10bit
			break;
		case 4:
			if( toLinear ) format = GL_SRGB8_ALPHA8;
			else  format = GL_RGBA;
			break;
		}

		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		setTexParam(GL_LINEAR_MIPMAP_LINEAR, GL_REPEAT);
		// level : 0 mipmap의 가장큰 level, 나머지는 알아서 생성
		// internalformat: 파일에 저장된값, format: 사용할값
		// 따라서 srgb에서 rgb로 선형공간으로 색이 이동되고 계산된후 다시 감마보정을 해준다.
		/* srgb -> rgb -> render -> srgb(glEnable(gl_framebuffer_srgb) */
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(buf);
		return texID;
	}

	/* c++17 global data member can initialize in declaration with inline keyword*/
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
}

#endif