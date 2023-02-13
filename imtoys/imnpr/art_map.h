//
//
//	2023-1-18 / im dong ye
//
//	TODO list:
//
//

#ifndef ART_MAP_H
#define ART_MAP_H

namespace lim
{
	class ArtMap: public TexBase
	{
	public:
		static constexpr GLint nr_mips = 4;
		GLint tone;
		std::string path;
	public:
		ArtMap(const std::string_view _path, GLint _tone)
			: TexBase(), tone(_tone)
		{
			path = _path;

			internal_format = GL_SRGB8;
			src_chanel_type = GL_UNSIGNED_BYTE;
			src_bit_per_channel = 8;

			glGenTextures(1, &tex_id);
			glBindTexture(GL_TEXTURE_2D, tex_id);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			// From: https://www.khronos.org/opengl/wiki/Common_Mistakes#Creating_a_complete_texture
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, nr_mips-1);

			void* data;
			for( int lv=0; lv<nr_mips; lv++ ) {
				int w, h, ch;
				std::string leveledPath = path;
				leveledPath.insert(path.rfind('.'), 1, '0'+nr_mips-1-lv);

				data=stbi_load(leveledPath.c_str(), &w, &h, &ch, 0);
				if( !data ) {
					Logger::get(1).log("[error]texture failed to load at path: %s\n", path.c_str());
					return;
				}
				if( lv==0 ) {
					width = w; height = h; nr_channels = ch;
				}
				aspect_ratio = width/(float)height;

				switch( ch ) {
					case 1: src_format = GL_RED; break;
					case 2: src_format = GL_RG; break;
					case 3: src_format = GL_RGB; break;
					case 4: src_format = GL_RGBA; break;
					default: Logger::get().log("[error] texter channels is over 4\n"); return;
				}
				printf("%s loaded : lv:%d, texID:%d, %dx%d, nrCh:%d\n"
					   , leveledPath.c_str(), lv, tex_id, w, h, ch);
				glTexImage2D(GL_TEXTURE_2D, lv, internal_format, w, h
							 , 0, src_format, src_chanel_type, data);

				stbi_image_free(data);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, nr_mips-1);
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};
}

#endif