//
//  2022-11-14 / im dong ye
//	edit HDRView by pf Hyun Joon Shin
//
//	Todo:
//	1. ARB 확장이 뭐지 어셈블리?
//  2. GL_TEXTURE_MAX_ANISOTROPY_EXT
//	3. texture loading 분리
//

#ifndef TEXTURE_H
#define TEXTURE_H

namespace lim
{
	class Texture
	{
	public:
		GLuint tex_id=0;
		int width=0, height=0;
		float aspect_ratio;
		int nr_channels=0;
		std::string tag;
		std::string path;
		// for simp mesh export
		const char *internal_model_path = nullptr;
		const char *format;
		const char *name;
		// 내부 저장 포맷, sRGB면 감마 변환
		GLint internal_format; 
		GLenum src_format, src_chanel_type;
		int bit_per_channel;
	private:
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
	protected:
		Texture(){}
	public:
		/* load texture */
		// internalFormat : GL_RGB32F(default), GL_RGB8, GL_SRGB8
		Texture(const std::string_view _path, GLint internalFormat=GL_RGB32F)
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
			aspect_ratio = width/(float)height;

			switch( nr_channels ) {
				case 1: src_format = GL_RED; break;
				case 2: src_format = GL_RG; break;
				case 3: src_format = GL_RGB; break;
				case 4: src_format = GL_RGBA; break;
				default: Logger::get().log("[error] texter channels is over 4\n");
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
		virtual ~Texture()
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
		void setMinMag(GLint magParam, GLint minParam)
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		// GL_CLAMP_TO_EDGE , GL_REPEAT , GL_REPEAT_MIRROR
		void setWrap(GLint param=GL_CLAMP_TO_EDGE)
		{
			glBindTexture(GL_TEXTURE_2D, tex_id);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, param);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, param);
			glBindTexture(GL_TEXTURE_2D, 0);
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

	static void texIDToFramebuffer(GLuint texID, const Framebuffer& fb, float gamma=2.2f)
	{
		fb.bind();

		Program& to_quad_prog = *AssetLib::get().to_quad_prog;
		to_quad_prog.use();

		glBindTexture(GL_TEXTURE_2D, texID);
		glActiveTexture(GL_TEXTURE0);
		
		setUniform(to_quad_prog.pid, "tex", 0);
		setUniform(to_quad_prog.pid, "gamma", gamma);

		glBindVertexArray(AssetLib::get().quad_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		fb.unbind();
	}
}

#endif
