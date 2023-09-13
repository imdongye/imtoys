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
	void TexBase::clear()
	{
		if( tex_id>0 ) {
			glDeleteTextures(1, &tex_id);
			tex_id=0;
		}
	}
	void TexBase::initGL(void* data)
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
	void TexBase::bind(GLuint pid, GLuint activeSlot, const std::string_view shaderUniformName) const
	{
		glActiveTexture(GL_TEXTURE0 + activeSlot);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		setUniform(pid, shaderUniformName, (int)activeSlot);
	}


	TexBase* makeTextureFromImage(std::string_view path, GLint internalFormat)
	{
		TexBase* rst = new TexBase();
		TexBase& tex = *rst;
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
			delete rst;
			return nullptr;
		}
		
		tex.internal_format = internalFormat;
		tex.path = path;
		tex.format = tex.path.c_str()+tex.path.rfind('.')+1;
		tex.name = tex.path.c_str()+path.rfind('\\')+path.rfind('/')+2;
		tex.aspect_ratio = tex.width/(float)tex.height;

		switch( tex.nr_channels ) {
			case 1: tex.src_format = GL_RED; break;
			case 2: tex.src_format = GL_RG; break;
			case 3: tex.src_format = GL_RGB; break;
			case 4: tex.src_format = GL_RGBA; break;
			log::err("texter channels is over 4\n");
			delete rst;
			stbi_image_free(data);
			return nullptr;
		}

		tex.initGL(data);
		stbi_image_free(data);

		return rst;
	}
}

namespace lim
{
	TexBase* makeTextureFromImage(std::string_view path, GLint internalFormat)
	{
		TexBase* rst = new TexBase();
		TexBase& tex = *rst;
		void* data;

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
			delete rst;
			return nullptr;
		}

		switch( tex.nr_channels ) {
			case 1: tex.src_format = GL_RED; break;
			case 2: tex.src_format = GL_RG; break;
			case 3: tex.src_format = GL_RGB; break;
			case 4: tex.src_format = GL_RGBA; break;
			default: {
				log::err("texter channels is over 4\n");
				delete rst;
				stbi_image_free(data);
				return nullptr;
			}
		}

		tex.internal_format = internalFormat;
		tex.path = path;
		tex.format = tex.path.c_str()+tex.path.rfind('.')+1;
		tex.name = tex.path.c_str()+path.rfind('\\')+path.rfind('/')+2;
		tex.aspect_ratio = tex.width/(float)tex.height;

		tex.initGL(data);
		stbi_image_free(data);

		return rst;
	}

	TexBase* makeTextureFromImageAuto(std::string_view path, bool convertLinear, int nrChannels, int bitPerChannel)
	{
		TexBase* rst = new TexBase();
		TexBase& tex = *rst;
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
			delete rst;
			return nullptr;
		}

		switch( tex.nr_channels ) {
			case 1: tex.src_format = GL_RED; break;
			case 2: tex.src_format = GL_RG; break;
			case 3: tex.src_format = GL_RGB; break;
			case 4: tex.src_format = GL_RGBA; break;
			default: {
				log::err("texter channels is over 4\n");
				delete rst;
				stbi_image_free(data);
				return nullptr;
			}
		}

		/*** auto internal format ***/
		tex.internal_format = -1;

		if( bitPerChannel==0 )
			bitPerChannel = tex.src_bit_per_channel;
		if( nrChannels==0 )
			nrChannels = tex.nr_channels;

		// sRGB는 밝은영역을 압축하기 위함이라 8비트 이상이 필요없음
		// From : https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		// Todo: GL_SRGB == GL_SRGB8 ?
		// 		SNORM은 언제 쓰지?
		//		I, UI, F 가 무슨 의미가 있지?
		if( convertLinear ) {
			if(nrChannels==3) tex.internal_format = GL_SRGB8;
			if(nrChannels==4) tex.internal_format = GL_SRGB8_ALPHA8;
		}
		else {
			if(nrChannels==1&&bitPerChannel==8) tex.internal_format = GL_R8;
			if(nrChannels==2&&bitPerChannel==8) tex.internal_format = GL_RG8;
			if(nrChannels==3&&bitPerChannel==8) tex.internal_format = GL_RGB8;
			if(nrChannels==4&&bitPerChannel==8) tex.internal_format = GL_RGBA8;
			if(nrChannels==1&&bitPerChannel==16) tex.internal_format = GL_R16F;
			if(nrChannels==2&&bitPerChannel==16) tex.internal_format = GL_RG16F;
			if(nrChannels==3&&bitPerChannel==16) tex.internal_format = GL_RGB16F;
			if(nrChannels==4&&bitPerChannel==16) tex.internal_format = GL_RGBA16F;
			if(nrChannels==1&&bitPerChannel==32) tex.internal_format = GL_R32F;
			if(nrChannels==2&&bitPerChannel==32) tex.internal_format = GL_RG32F;
			if(nrChannels==3&&bitPerChannel==32) tex.internal_format = GL_RGB32F;
			if(nrChannels==4&&bitPerChannel==32) tex.internal_format = GL_RGBA32F;
		}

		tex.path = path;
		tex.format = tex.path.c_str()+tex.path.rfind('.')+1;
		tex.name = tex.path.c_str()+path.rfind('\\')+path.rfind('/')+2;
		tex.aspect_ratio = tex.width/(float)tex.height;

		tex.initGL(data);
		stbi_image_free(data);

		return rst;
	}
	
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