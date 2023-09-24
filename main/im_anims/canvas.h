//
//	2022-03-13 / im dong ye
//
//	TODO list:
//

#ifndef __canvas_h_
#define __canvas_h_

#include <limbrary/texture.h>
#include <vector>
#include <glm/glm.hpp>

namespace lim
{
	class CanvasGray : public TexBase
	{
	public:
		float clear_color = 0.01f;
		std::vector<float> buf;

	public:
		CanvasGray(int w, int h) : TexBase()
		{
			internal_format = GL_R32F;
			src_format = GL_RED;
			src_chanel_type = GL_FLOAT;
			src_bit_per_channel = 32;

			mag_filter = GL_NEAREST;
			min_filter = GL_NEAREST;
			mipmap_max_level = 0;

			resize(w, h);
			initGL(buf.data());
		}
		virtual ~CanvasGray()
		{
		}
		void clear()
		{
			std::fill(buf.begin(), buf.end()-1, clear_color);
		}
		void resize(int w, int h)
		{
			width = w;
			height = h;
			buf.resize(w * h);
			clear();

			initGL(buf.data());
		}
		void update()
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, buf.data());
		}
		float &at(int x, int y)
		{
			x = glm::min(width - 1, glm::max(0, x));
			y = glm::min(height - 1, glm::max(0, y));
			return buf[x + y * width];
		}
	};

	class CanvasColor : public TexBase
	{
	public:
		glm::vec3 clear_color = {0.f, 0.1f, 0.12f};
		std::vector<glm::vec3> buf;
	public:
		CanvasColor(int w, int h) : TexBase()
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
		}
		void clear()
		{
			// memset(M.data(), 0, width*height*sizeof(glm::vec3)); // max30.1fps
			// std::fill_n(M.data(), width*height, c); // max26.9fps
			std::fill(buf.begin(), buf.end() - 1, clear_color); // max27fps
		}
		void resize(int w, int h)
		{
			width = w;
			height = h;
			buf.resize(w * h);
			clear();
			initGL(buf.data());
		}
		void update()
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, src_format, src_chanel_type, buf.data());
		}
		glm::vec3 &at(int x, int y)
		{
			x = glm::min(width - 1, glm::max(0, x));
			y = glm::min(height - 1, glm::max(0, y));
			return buf[x + y * width];
		}
	};
}

#endif