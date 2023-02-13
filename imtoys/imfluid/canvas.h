
//
//	2022-03-13 / im dong ye
//
//	TODO list:
//

#ifndef CANVAS_H
#define CANVAS_H

#include "../limbrary/texture.h"

namespace lim
{
	class CanvasGray: public TexBase
	{
	public:
		float* M=nullptr;
	public:
		CanvasGray(int w, int h):TexBase()
		{
			internal_format = GL_R32F;
			src_format = GL_RED;
			src_chanel_type = GL_FLOAT;

			mag_filter = GL_NEAREST;
			min_filter = GL_NEAREST;
			mipmap_max_level = 0;

			resize(w, h);
		}
		virtual ~CanvasGray()
		{
			delete M;
		}
		void resize(int w, int h)
		{
			width = w; height = h;
			if( !M ) delete M;
			M = new float[w*h];

			create(M);
		}
		void update()
		{
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, M);
		}
		float& at(int x, int y)
		{
			x = glm::min(width-1, glm::max(0, x));
			y = glm::min(height-1, glm::max(0, height-1-y));
			return M[x+y*width];
		}
	};

	//Todo
	class CanvasColor: public TexBase
	{
	public:
		std::vector<glm::vec3> M;

	public:
		CanvasColor(int w, int h):TexBase()
		{
			internal_format = GL_RGB32F;
			src_format = GL_RGB;
			src_chanel_type = GL_FLOAT;

			mag_filter = GL_NEAREST;
			min_filter = GL_NEAREST;
			mipmap_max_level = 0;

			width = w; height = h;
			// From: https://stackoverflow.com/questions/20106173/glteximage2d-and-null-data
			// no pixel transfer when pass null
			create(nullptr);
		}
	};
}

#endif