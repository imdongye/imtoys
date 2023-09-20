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
	void copyTexBaseProps(const lim::TexBase& src, lim::TexBase& dst) {
		dst.name 				= src.name+"-copied";
		dst.width 				= src.width;
		dst.height 				= src.height;
		dst.aspect_ratio 		= src.aspect_ratio;
		dst.internal_format 	= src.internal_format;
		dst.mag_filter 			= src.mag_filter;
		dst.min_filter 			= src.min_filter;
		dst.mipmap_max_level 	= src.mipmap_max_level;
		dst.nr_channels 		= src.nr_channels;
		dst.src_bit_per_channel	= src.src_bit_per_channel;
		dst.src_chanel_type 	= src.src_chanel_type;
		dst.src_format 			= src.src_format;
		dst.wrap_param 			= src.wrap_param;
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
	
	Texture* Texture::clone()
	{
		Texture* rst = new Texture();
		Texture& dup = *rst;
		
		copyTexBaseProps(dup, *this);
		dup.path = path;
		dup.format = dup.path.c_str()+(format-path.c_str());
		dup.initFromImage(path, internal_format);

	// From: https://jamssoft.tistory.com/235
	// From2: https://stackoverflow.com/questions/23981016/best-method-to-copy-texture-to-texture
	/* Todo: Fbo to Tex
		GLuint srcFbo;
		// Wrap destination texture to fbo
		glGenBuffers( 1, &srcFbo );
		glBindFramebuffer( GL_FRAMEBUFFER, srcFbo );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_id, 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		glGenTextures(1, &dup.tex_id);
		glBindTexture(GL_TEXTURE_2D, dup.tex_id);
		setTexParam(*this);

		// readBuffer[GL_READ_BUFFER] is automaticaly changed to GL_BACK on default framebuffer, and
		// GL_COLOR_ATTACHMENT0 on non-zero framebuffer.
		// so glReadBuffer call is not needed
		//glReadBuffer( GL_FRONT );
		glCopyTexImage2D( GL_TEXTURE_2D, 0, internal_format, 0, 0, width, height, 0 );
		glGenerateMipmap( GL_TEXTURE_2D );

		glBindFramebuffer(GL_FRAMEBUFFER, 0 );
		glBindTexture( GL_TEXTURE_2D, 0 );

		if( srcFbo ) {
			glDeleteFramebuffers( 1, &srcFbo );
		}
	*/

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

		log::pure("texture %s loaded\n", path.c_str());

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
			log::err("texture failed to load at path: %s\n", path.c_str());
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
			if(nrChannels==1&&bitPerChannel==8)  {internal_format = GL_R8; 		log::pure("texture %s loaded auto with GL_R8\n", path.c_str()); }
			if(nrChannels==2&&bitPerChannel==8)  {internal_format = GL_RG8; 	log::pure("texture %s loaded auto with GL_RG8\n", path.c_str()); }
			if(nrChannels==3&&bitPerChannel==8)  {internal_format = GL_RGB8; 	log::pure("texture %s loaded auto with GL_RGB8\n", path.c_str()); }
			if(nrChannels==4&&bitPerChannel==8)  {internal_format = GL_RGBA8; 	log::pure("texture %s loaded auto with GL_RGBA8\n", path.c_str()); }
			if(nrChannels==1&&bitPerChannel==16) {internal_format = GL_R16F; 	log::pure("texture %s loaded auto with GL_R16F\n", path.c_str()); }
			if(nrChannels==2&&bitPerChannel==16) {internal_format = GL_RG16F; 	log::pure("texture %s loaded auto with GL_RG16F\n", path.c_str()); }
			if(nrChannels==3&&bitPerChannel==16) {internal_format = GL_RGB16F; 	log::pure("texture %s loaded auto with GL_RGB16F\n", path.c_str()); }
			if(nrChannels==4&&bitPerChannel==16) {internal_format = GL_RGBA16F; log::pure("texture %s loaded auto with GL_RGBA16F\n", path.c_str()); }
			if(nrChannels==1&&bitPerChannel==32) {internal_format = GL_R32F; 	log::pure("texture %s loaded auto with GL_R32F\n", path.c_str()); }
			if(nrChannels==2&&bitPerChannel==32) {internal_format = GL_RG32F; 	log::pure("texture %s loaded auto with GL_RG32F\n", path.c_str()); }
			if(nrChannels==3&&bitPerChannel==32) {internal_format = GL_RGB32F; 	log::pure("texture %s loaded auto with GL_RGB32F\n", path.c_str()); }
			if(nrChannels==4&&bitPerChannel==32) {internal_format = GL_RGBA32F; log::pure("texture %s loaded auto with GL_RGBA32F\n", path.c_str()); }
		}

		format = path.c_str()+path.rfind('.')+1;
		name = std::string(path.c_str()+path.rfind('\\')+path.rfind('/')+2);
		aspect_ratio = width/(float)height;

		initGL(data);
		stbi_image_free(data);

		return true;
	}
	
}