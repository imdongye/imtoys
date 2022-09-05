//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
#ifndef TEXTURE_H
#define TEXTURE_H

#include "limclude.h"

namespace lim
{
	struct Texture
	{
		GLuint id;
		std::string type;
		std::string path; // relative path+filename or only filename
	};

	static inline GLuint loadTextureFromFile(const char* cpath, bool toLinear = true)
	{
		std::string spath = std::string(cpath);

		GLuint texID=0;
		glGenTextures(1, &texID);

		int w, h, channels;
		// 0 => comp 있는대로
		void* buf = stbi_load(cpath, &w, &h, &channels, 4); // todo: 0

		if( !buf )
		{
			fprintf(stderr, "texture failed to load at path: %s\n", cpath);
			stbi_image_free(buf);
			return texID;
		}
		else
		{
			fprintf(stdout, "texture loaded : %s , %dx%d, %d channels\n", cpath, w, h, channels);
		}

		// load into vram
		GLenum format = GL_RGBA;
		switch( channels )
		{
		case 1: format = GL_ALPHA; break;
		case 2: format = 0; break;
		case 3:
			if( !toLinear ) format = GL_RGB;
			else format = GL_SRGB8; // if hdr => 10bit
			break;
		case 4:
			if( !toLinear ) format = GL_RGBA;
			else format = GL_SRGB8_ALPHA8; // if hdr => 10bit
			break;
		}

		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		// 0~1 반복 x
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//GL_REPEAT GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// GL_NEAREST : texel만 읽음
		// GL_LINEAR : 주변점 선형보간
		// texture을 키울때는 선형보간말고 다른방법 없음.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// GL_LINEAR_MIPMAP_LINEAR : mipmap에서 찾아서 4점을 보간하고 다른 mipmap에서 찾아서 또 섞는다.
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		// level : 0 mipmap의 가장큰 level, 나머지는 알아서 생성
		// internalformat: 파일에 저장된값, format: 사용할값
		// 따라서 srgb에서 rgb로 선형공간으로 색이 이동되고 계산된후 다시 감마보정을 해준다.
		/* srgb -> rgb -> render -> srgb(glEnable(gl_framebuffer_srgb) */
		glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(buf);
		return texID;
	}

}

#endif