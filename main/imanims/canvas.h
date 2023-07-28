
//
//	2022-03-13 / im dong ye
//
//	TODO list:
//

#ifndef CANVAS_H
#define CANVAS_H

#include <limbrary/texture.h>

namespace lim
{
	class CanvasGray: public TexBase
	{
	public:
		std::vector<float> M;
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
			M.clear();
		}
		void resize(int w, int h)
		{
			width = w; height = h;
			M.resize(w*h);
			clear();

			create(M.data());
		}
		void update()
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, M.data());
		}
		void clear(const float c = 0.f)
		{
			std::fill(M.begin(), M.end()-1, c);
		}
		float& at(int x, int y)
		{
			x = glm::min(width-1, glm::max(0, x));
			y = glm::min(height-1, glm::max(0, y));
			return M[x+y*width];
		}
		
	};

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

			resize(w, h);
		}
		virtual ~CanvasColor()
		{
			M.clear();
		}
		void resize(int w, int h)
		{
			width = w; height = h;
			M.resize(w*h);
			clear();
			create(M.data());
		}
		void update()
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, M.data());
		}
		void clear(const glm::vec3& c= {0.f,0.1f,0.12f})
		{
			//memset(M.data(), 0, width*height*sizeof(glm::vec3)); // max30.1fps
			//std::fill_n(M.data(), width*height, c); // max26.9fps
			std::fill(M.begin(), M.end()-1, c); // max27fps
		}
		glm::vec3& at(int x, int y)
		{
			x = glm::min(width-1, glm::max(0, x));
			y = glm::min(height-1, glm::max(0, y));
			return M[x+y*width];
		}
	};
}

#endif