//
//  2022-11-14 / im dong ye
//	edit HDRView by pf Hyun Joon Shin
//
//	Todo:
//	1. ARB 확장이 뭐지 어셈블리?
//  2. GL_TEXTURE_MAX_ANISOTROPY_EXT
//	3. texture loading 분리
//

#include <limbrary/texture.h>
#include <limbrary/program.h>
#include <limbrary/log.h>
#include <limbrary/asset_lib.h>
#include <stb_image.h>

namespace lim
{
	TexBase::TexBase(GLint internalFormat)
	{
		internal_format = internalFormat;
	}
	TexBase::~TexBase()
	{
		clear();
	}
	void TexBase::create(int _width, int _height, GLint internalFormat, GLuint srcFormat, GLuint srcChanelType, void* data)
	{
		if( width!=_width||height!=_height||internal_format!=internalFormat ) 
			clear();

		width = _width;
		height = _height;
		internal_format = internalFormat;
		src_format = srcFormat;
		src_chanel_type=srcChanelType;

		create(data);
	}
	void TexBase::create(void* data)
	{
		glGenTextures(1, &tex_id);
		glBindTexture(GL_TEXTURE_2D, tex_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, mipmap_max_level);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_chanel_type, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	void TexBase::clear()
	{
		if( tex_id>0 ) {
			glDeleteTextures(1, &tex_id);
			tex_id=0;
		}
	}
	void TexBase::bind(GLuint pid, GLuint activeSlot, const std::string_view shaderUniformName) const
	{
		glActiveTexture(GL_TEXTURE0 + activeSlot);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		setUniform(pid, shaderUniformName, (int)activeSlot);
	}


	bool loadImageToTex(std::string_view path, TexBase& tex)
	{
		void *data;

		// hdr loading
		if( stbi_is_hdr(path.data()) ) {
			data=stbi_loadf(path.data(), &tex.width, &tex.height, &tex.nr_channels, 0);
			if( stbi_is_16_bit(path.data()) ) {
				tex.src_chanel_type = GL_HALF_FLOAT;
				tex.src_bit_per_channel = 16;
			}
			else {
				tex.src_chanel_type = GL_FLOAT;
				tex.src_bit_per_channel = 32;
			}
		}
		// ldr loading
		else {
			data=stbi_load(path.data(), &tex.width, &tex.height, &tex.nr_channels, 0);
			tex.src_chanel_type = GL_UNSIGNED_BYTE;
			tex.src_bit_per_channel = 8;
		}
		if( !data ) {
			log::err("texture failed to load at path: %s\n", path.data());
			return false;
		}

		tex.aspect_ratio = tex.width/(float)tex.height;

		switch( tex.nr_channels ) {
			case 1: tex.src_format = GL_RED; break;
			case 2: tex.src_format = GL_RG; break;
			case 3: tex.src_format = GL_RGB; break;
			case 4: tex.src_format = GL_RGBA; break;
			log::err("texter channels is over 4\n");
		}

		tex.create(data);
		stbi_image_free(data);

		return true;
	}


	/* load texture */
	// internalFormat : GL_RGB32F(default), GL_RGB8, GL_SRGB8
	Texture::Texture(const std::string_view _path, GLint internalFormat):TexBase()
		, path(_path), format(path.c_str()+path.rfind('.')+1)
		, name(path.c_str()+path.rfind('\\')+path.rfind('/')+2)
	{
		internal_format = internalFormat;
		loadImageToTex(path, *this);
		log::info("%s loaded : texID:%d, %dx%d, nrCh:%d, bit:%d, fm:%s, aspect:%.3f\n", path.c_str(), tex_id, width, height, nr_channels, src_bit_per_channel, format, width/(float)height);
	}
	Texture::~Texture()
	{
		log::info("texture clear\n");
		clear();
	}
	std::shared_ptr<Texture> Texture::clone()
	{
		return std::make_shared<Texture>(path, internal_format);
	}
	void Texture::reload(const std::string_view _path, GLint internalFormat)
	{
		clear();
		path = _path; internal_format = internalFormat;
		loadImageToTex(_path, *this);
	}
	
	/* do not use below for multisampling framebuffer */
	void Tex2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, w, h);
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		Program& to_quad_prog = *AssetLib::get().tex_to_quad_prog;
		to_quad_prog.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);

		setUniform(to_quad_prog.pid, "tex", 0);
		setUniform(to_quad_prog.pid, "gamma", gamma);

		glBindVertexArray(AssetLib::get().quad_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void Gray2Fbo(GLuint texID, GLuint fbo, int w, int h, float gamma)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, w, h);
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_MULTISAMPLE);

		Program& to_quad_prog = *AssetLib::get().gray_to_quad_prog;
		to_quad_prog.use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texID);

		setUniform(to_quad_prog.pid, "tex", 0);
		setUniform(to_quad_prog.pid, "gamma", gamma);

		glBindVertexArray(AssetLib::get().quad_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
}