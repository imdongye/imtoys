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
#include <limbrary/asset_lib.h>
#include <limbrary/log.h>
#include <stb_image.h>

namespace
{
	void copyTexBaseProps(const lim::TexBase& src, lim::TexBase& dst) {
		dst.name 				= src.name;
		dst.width 				= src.width;
		dst.height 				= src.height;
		dst.aspect_ratio 		= src.aspect_ratio;
		dst.nr_channels 		= src.nr_channels;
		dst.internal_format 	= src.internal_format;
		dst.src_format 			= src.src_format;
		dst.src_chanel_type 	= src.src_chanel_type;
		dst.src_bit_per_channel	= src.src_bit_per_channel;
		dst.mag_filter 			= src.mag_filter;
		dst.min_filter 			= src.min_filter;
		dst.wrap_param 			= src.wrap_param;
		dst.mipmap_max_level 	= src.mipmap_max_level;
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
	TexBase::TexBase(const TexBase& src)
	{
		copyTexBaseProps(src, *this);
		initGL();

		copyTexToTex(src.tex_id, *this);
	}
	TexBase::TexBase(TexBase&& src) noexcept
	{
		copyTexBaseProps(src, *this);
		tex_id = src.tex_id;
		src.tex_id = 0;
	}
	TexBase& TexBase::operator=(TexBase&& src) noexcept
	{
		if(this == &src)
			return *this;
		deinitGL();

		copyTexBaseProps(src, *this);
		tex_id = src.tex_id;
		src.tex_id = 0;
		return *this;
	}
	TexBase::~TexBase() noexcept
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



	Texture::Texture()
	{
	}
	Texture::Texture(const Texture& src)
		: TexBase(src)
	{
		path = src.path;
		format = path.c_str()+(src.format-src.path.c_str());
	}
	Texture::Texture(Texture&& src) noexcept
		:TexBase(std::move(src))
	{
		path = std::move(src.path);
		format = path.c_str()+(src.format-src.path.c_str());
	}
	Texture& Texture::operator=(Texture&& src) noexcept
	{
		if(this == &src)
			return *this;
		TexBase::operator=(std::move(src));
		path = std::move(src.path);
		format = path.c_str()+(src.format-src.path.c_str());
		return *this;
	}
	Texture::~Texture() noexcept
	{
	}
	bool Texture::initFromImage(std::string_view _path, GLint internalFormat)
	{
		path = _path;
		void* data;

		stbi_set_flip_vertically_on_load(true);

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
		name = std::string(path.c_str()+path.find_last_of("/\\")+2);
		aspect_ratio = width/(float)height;

		initGL(data);
		stbi_image_free(data);

		log::pure("texture %s loaded\n", path.c_str()+path.find_last_of("/\\")+1);

		return true;
	}

	bool Texture::initFromImageAuto(std::string_view _path, bool convertLinear, int nrChannels, int bitPerChannel)
	{
		path = _path;
		void *data;

		stbi_set_flip_vertically_on_load(true);

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
		name = std::string(path.c_str()+path.find_last_of("/\\")+2);
		aspect_ratio = width/(float)height;

		initGL(data);
		stbi_image_free(data);

		log::pure("texture %s loaded auto\n", path.c_str()+path.find_last_of("/\\")+1);

		return true;
	}
	

	
	void drawTexToQuad(const GLuint texId, float gamma) 
	{
		const Program& prog = AssetLib::get().tex_to_quad_prog;

		prog.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texId);
		prog.bind("tex", 0);
		prog.bind("gamma", gamma);

		glBindVertexArray(AssetLib::get().screen_quad.vert_array);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, AssetLib::get().screen_quad.element_buf);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void copyTexToTex(const GLuint srcTexId, TexBase& dstTex) 
	{
		GLuint srcFbo = 0;
		glGenFramebuffers(1, &srcFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, srcFbo);
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srcTexId, 0 );

		glBindTexture(GL_TEXTURE_2D, dstTex.tex_id);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, dstTex.width, dstTex.height);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glDeleteFramebuffers( 1, &srcFbo );
	}
	void copyTexToTex(const TexBase& srcTex, TexBase& dstTex) 
	{
		if(dstTex.tex_id==0) {
			copyTexBaseProps(srcTex, dstTex);
			dstTex.initGL();
		}

		GLuint srcFbo = 0;
		glGenFramebuffers( 1, &srcFbo );
		glBindFramebuffer( GL_FRAMEBUFFER, srcFbo );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, srcTex.tex_id, 0 );
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		GLuint dstFbo = 0;
		glGenFramebuffers( 1, &dstFbo );
		glBindFramebuffer( GL_FRAMEBUFFER, dstFbo );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex.tex_id, 0 );
		glDrawBuffer(GL_COLOR_ATTACHMENT0);


		glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);
		glBlitFramebuffer(0, 0, srcTex.width, srcTex.height, 0, 0, dstTex.width, dstTex.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &srcFbo);
		glDeleteFramebuffers(1, &dstFbo);

		glBindTexture(GL_TEXTURE_2D, dstTex.tex_id);
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}