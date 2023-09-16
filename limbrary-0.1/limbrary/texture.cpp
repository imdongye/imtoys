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
#include <stb_image.h>

namespace
{
	void copyTexBaseProps(const lim::TexBase& from, lim::TexBase& to) {
		to.name = from.name+"-copied";
		to.width = from.width;
		to.height = from.height;
		to.aspect_ratio = from.aspect_ratio;
		to.internal_format = from.internal_format;
		to.mag_filter = from.mag_filter;
		to.min_filter = from.min_filter;
		to.mipmap_max_level = from.mipmap_max_level;
		to.nr_channels = from.nr_channels;
		to.src_bit_per_channel = from.src_bit_per_channel;
		to.src_chanel_type = from.src_chanel_type;
		to.src_format = from.src_format;
		to.wrap_param = from.wrap_param;
	}
	void setTexParam(const lim::TexBase& tex) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex.mag_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex.min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex.wrap_param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex.wrap_param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex.mipmap_max_level);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);
	}
}

namespace lim
{
	TexBase::TexBase()
	{
	}
	TexBase::~TexBase()
	{
		deinitGL();
	}
	void TexBase::deinitGL()
	{
		if( tex_id>0 ) {
			glDeleteTextures(1, &tex_id);
			tex_id=0;
		}
	}
	void TexBase::initGL(void* data)
	{
		deinitGL();
		glGenTextures(1, &tex_id);
		glBindTexture(GL_TEXTURE_2D, tex_id);

		setTexParam(*this);

		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, src_format, src_chanel_type, data);
		
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void TexBase::bind(GLuint pid, GLuint activeSlot, const std::string_view shaderUniformName) const
	{
		glActiveTexture(GL_TEXTURE0 + activeSlot);
		glBindTexture(GL_TEXTURE_2D, tex_id);
		setUniform(pid, shaderUniformName, (int)activeSlot);
	}
	TexBase* TexBase::clone()
	{
		TexBase* rst = new TexBase();
		TexBase& tex = *rst;
		
		copyTexBaseProps(*rst, *this);

		glGenTextures(1, &tex.tex_id);
		glBindTexture(GL_TEXTURE_2D, tex.tex_id);
		setTexParam(*this);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, internal_format, 0, 0, width, height, 0);

		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		return rst;
	}
	Texture* Texture::clone()
	{
		Texture* rst = new Texture();
		Texture& tex = *rst;
		
		copyTexBaseProps(*rst, *this);
		tex.path = path;
		tex.format = tex.path.c_str()+(format-path.c_str());
		tex.tag = tag;

		glGenTextures(1, &tex.tex_id);
		glBindTexture(GL_TEXTURE_2D, tex.tex_id);
		setTexParam(*this);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, internal_format, 0, 0, width, height, 0);

		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		return rst;
	}
	bool Texture::initFromImage(std::string_view _path, GLint internalFormat)
	{
		path = _path;
		void* data;
		deinitGL();

		// hdr loading
		if( stbi_is_hdr(path.data()) ) {
			data=stbi_loadf(path.data(), &width, &height, &nr_channels, 0);
			if( stbi_is_16_bit(path.data()) ) {
				src_chanel_type = GL_HALF_FLOAT;
				src_bit_per_channel = 16;
			}
			else {
				src_chanel_type = GL_FLOAT;
				src_bit_per_channel = 32;
			}
		}
		// ldr loading
		else {
			data=stbi_load(path.data(), &width, &height, &nr_channels, 0);
			src_chanel_type = GL_UNSIGNED_BYTE;
			src_bit_per_channel = 8;
		}
		if( !data ) {
			log::err("texture failed to load at path: %s\n", path.data());
			return false;
		}

		switch( nr_channels ) {
			case 1: src_format = GL_RED; break;
			case 2: src_format = GL_RG; break;
			case 3: src_format = GL_RGB; break;
			case 4: src_format = GL_RGBA; break;
			default: {
				log::err("texter channels is over 4\n");
				stbi_image_free(data);
				return false;
			}
		}

		internal_format = internalFormat;
		format = path.c_str()+path.rfind('.')+1;
		name = std::string(path.c_str()+path.rfind('\\')+path.rfind('/')+2);
		aspect_ratio = width/(float)height;

		initGL(data);
		stbi_image_free(data);

		return true;
	}

	bool Texture::initFromImageAuto(std::string_view _path, bool convertLinear, int nrChannels, int bitPerChannel)
	{
		path = _path;
		void *data;
		deinitGL();

		// hdr loading
		if( stbi_is_hdr(path.data()) ) {
			data=stbi_loadf(path.data(), &width, &height, &nr_channels, 0);
			if( stbi_is_16_bit(path.data()) ) {
				src_chanel_type = GL_HALF_FLOAT;
				src_bit_per_channel = 16;
			}
			else {
				src_chanel_type = GL_FLOAT;
				src_bit_per_channel = 32;
			}
		}
		// ldr loading
		else {
			data=stbi_load(path.data(), &width, &height, &nr_channels, 0);
			src_chanel_type = GL_UNSIGNED_BYTE;
			src_bit_per_channel = 8;
		}
		if( !data ) {
			log::err("texture failed to load at path: %s\n", path.data());
			return false;
		}

		switch( nr_channels ) {
			case 1: src_format = GL_RED; break;
			case 2: src_format = GL_RG; break;
			case 3: src_format = GL_RGB; break;
			case 4: src_format = GL_RGBA; break;
			default: {
				log::err("texter channels is over 4\n");
				stbi_image_free(data);
				return false;
			}
		}

		/*** auto internal format ***/
		internal_format = -1;

		if( bitPerChannel==0 )
			bitPerChannel = src_bit_per_channel;
		if( nrChannels==0 )
			nrChannels = nr_channels;

		// sRGB는 밝은영역을 압축하기 위함이라 8비트 이상이 필요없음
		// From : https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		// Todo: GL_SRGB == GL_SRGB8 ?
		// 		SNORM은 언제 쓰지?
		//		I, UI, F 가 무슨 의미가 있지?
		if( convertLinear ) {
			if(nrChannels==3) internal_format = GL_SRGB8;
			if(nrChannels==4) internal_format = GL_SRGB8_ALPHA8;
		}
		else {
			if(nrChannels==1&&bitPerChannel==8) internal_format = GL_R8;
			if(nrChannels==2&&bitPerChannel==8) internal_format = GL_RG8;
			if(nrChannels==3&&bitPerChannel==8) internal_format = GL_RGB8;
			if(nrChannels==4&&bitPerChannel==8) internal_format = GL_RGBA8;
			if(nrChannels==1&&bitPerChannel==16) internal_format = GL_R16F;
			if(nrChannels==2&&bitPerChannel==16) internal_format = GL_RG16F;
			if(nrChannels==3&&bitPerChannel==16) internal_format = GL_RGB16F;
			if(nrChannels==4&&bitPerChannel==16) internal_format = GL_RGBA16F;
			if(nrChannels==1&&bitPerChannel==32) internal_format = GL_R32F;
			if(nrChannels==2&&bitPerChannel==32) internal_format = GL_RG32F;
			if(nrChannels==3&&bitPerChannel==32) internal_format = GL_RGB32F;
			if(nrChannels==4&&bitPerChannel==32) internal_format = GL_RGBA32F;
		}

		format = path.c_str()+path.rfind('.')+1;
		name = std::string(path.c_str()+path.rfind('\\')+path.rfind('/')+2);
		aspect_ratio = width/(float)height;

		initGL(data);
		stbi_image_free(data);

		return true;
	}
	
}