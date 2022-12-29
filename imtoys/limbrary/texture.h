//
//  2022-11-14 / im dong ye
//	edit HDRView by pf Hyun Joon Shin
//
//	Todo:
//	1. ARB 확장이 뭐지 어셈블리?
//  2. GL_TEXTURE_MAX_ANISOTROPY_EXT
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include "ICC.h"

#define NOMINMAX
#include <libraw/libraw.h>

namespace lim
{
	class Texture
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
		const char* name;
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
			, name(path.c_str()+path.rfind('\\')+path.rfind('/')+2)
		{
			void *data;
			// hdr loading
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
			// ldr loading
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

			glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_chanel_type, data);
			stbi_image_free(data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glBindTexture(GL_TEXTURE_2D, 0);

			printf("%s loaded : texID:%d, %dx%d, nrCh:%d, bit:%d, fm:%s, aspect:%.3f\n", path.c_str(), tex_id, width, height, nr_channels, bit_per_channel, format, width/(float)height);
		}
		~Texture()
		{
			printf("texture clear\n");
			clear();
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

	class ColorAwareImage: public Texture
	{
	public:
		ICC::ColorProfile profile;
		glm::vec3 gamma;
		glm::mat3 rgbToPCS;
	public:
		ColorAwareImage(const std::string_view _path): Texture(_path, GL_RGB32F)
		{
			/* read meta data */
			LibRaw raw;
			raw.open_file(path.c_str());
			raw.unpack();
			Logger::get() << "< Meta data >" << Logger::endl;
			Logger::get() << "ISO : " << raw.imgdata.other.iso_speed << Logger::endl;
			Logger::get() << "Exposure Time : " << raw.imgdata.other.shutter << Logger::endl;
			Logger::get() << "Aperture : " << raw.imgdata.other.aperture << Logger::endl;
			Logger::get() << "Focal Lenth : " << raw.imgdata.other.focal_len << Logger::endl;
			Logger::get() << "Black Level : " << raw.imgdata.color.black << Logger::endl;
			Logger::get() << "Max Value : " << raw.imgdata.color.maximum << Logger::endl;
			Logger::get() << "RAW bit: " << raw.imgdata.color.raw_bps << Logger::endl;

			if( (strcmp(format, "jpg")==0||strcmp(format, "jpeg")==0) ) { // is JPEG
				profile.initWithJPEG(path, true);
				gamma = profile.gamma;
				//rgbToPCS = 
			}
			else { // default : srgb
				gamma = glm::vec3(2.4);
				// D65
				rgbToPCS = glm::mat3({0.4124565, 0.3575761, 0.1804375},
									 {0.2126729, 0.7151522, 0.0721750},
									 {0.0193339, 0.1191920, 0.9503041});
				rgbToPCS = glm::transpose(rgbToPCS);
			}
		}
		void toFramebuffer(const Framebuffer& fb)
		{
			fb.bind();

			Program& colorAwareProg = *AssetLib::get().toQuadProg;
			colorAwareProg.use();

			glBindTexture(GL_TEXTURE_2D, tex_id);
			glActiveTexture(GL_TEXTURE0);

			setUniform(colorAwareProg.pid, "tex", 0);
			//setUniform()

			glBindVertexArray(AssetLib::get().quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindVertexArray(0);

			fb.unbind();
		}
	};

	static void texIDToFramebuffer(GLuint texID, const Framebuffer& fb, float gamma=2.2f)
	{
		fb.bind();

		Program& toQuadProg = *AssetLib::get().toQuadProg;
		toQuadProg.use();

		glBindTexture(GL_TEXTURE_2D, texID);
		glActiveTexture(GL_TEXTURE0);
		
		setUniform(toQuadProg.pid, "tex", 0);

		glBindVertexArray(AssetLib::get().quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		fb.unbind();
	}
}

#endif
