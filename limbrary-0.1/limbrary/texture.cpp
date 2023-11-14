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
	void copyTexBaseProps(const lim::Texture& src, lim::Texture& dst) {
		dst.name 				= src.name;
		dst.width 				= src.width;
		dst.height 				= src.height;
		dst.internal_format 	= src.internal_format;
		dst.mag_filter 			= src.mag_filter;
		dst.min_filter 			= src.min_filter;
		dst.wrap_param 			= src.wrap_param;
		dst.mipmap_max_level 	= src.mipmap_max_level;

		dst.aspect_ratio 		= src.aspect_ratio;

		dst.file_path			= src.file_path;
		dst.file_format			= dst.file_path.c_str() + (src.file_format - src.file_path.c_str());
		dst.src_nr_channels     = src.src_nr_channels;
		dst.src_format 			= src.src_format;
		dst.src_chanel_type 	= src.src_chanel_type;
		dst.src_bit_per_channel	= src.src_bit_per_channel;
	}
}

namespace lim
{
	Texture::Texture()
	{
	}
	Texture::Texture(const Texture& src)
	{
		copyTexBaseProps(src, *this);
		initGL();
		copyTexToTex(src.tex_id, *this);
	}
	Texture::Texture(Texture&& src) noexcept
	{
		copyTexBaseProps(src, *this);
		tex_id = src.tex_id;
		src.tex_id = 0;
	}
	Texture& Texture::operator=(Texture&& src) noexcept
	{
		if(this == &src)
			return *this;
		deinitGL();
		copyTexBaseProps(src, *this);
		tex_id = src.tex_id;
		src.tex_id = 0;
		return *this;
	}
	Texture::~Texture() noexcept
	{
		deinitGL();
	}
	void Texture::deinitGL()
	{
		if( tex_id>0 ) {
			glDeleteTextures(1, &tex_id);
			tex_id=0;
		}
	}
	void Texture::initGL(void* data)
	{
		deinitGL();
		aspect_ratio = width/(float)height;

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
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void* Texture::getDataAndPropsFromFile(std::string_view filePath)
	{
		void* data;

		file_path = filePath;
		file_format = file_path.c_str()+file_path.rfind('.')+1;
		name = std::string(file_path.c_str()+file_path.find_last_of("/\\")+1);

		stbi_set_flip_vertically_on_load(true);

		// hdr loading
		if( stbi_is_hdr(file_path.c_str()) ) {
			data=stbi_loadf(file_path.c_str(), &width, &height, &src_nr_channels, 0);
			if( stbi_is_16_bit(file_path.c_str()) ) {
				src_chanel_type = GL_HALF_FLOAT;
				src_bit_per_channel = 16;
			}
			else {
				src_chanel_type = GL_FLOAT;
				src_bit_per_channel = 32;
			}
		}
		else {
			data=stbi_load(file_path.c_str(), &width, &height, &src_nr_channels, 0);
			src_chanel_type = GL_UNSIGNED_BYTE;
			src_bit_per_channel = 8;
		}
		if( !data ) {
			log::err("texture failed to load at path: %s\n", file_path.c_str());
			return nullptr;
		}

		switch( src_nr_channels ) {
			case 1: src_format = GL_RED; break;
			case 2: src_format = GL_RG; break;
			case 3: src_format = GL_RGB; break;
			case 4: src_format = GL_RGBA; break;
			default: {
				log::err("texter channels is over 4\n");
				stbi_image_free(data);
				return nullptr;
			}
		}
		return data;
	}
	bool Texture::initFromFile(std::string_view filePath)
	{
		void* data = getDataAndPropsFromFile(filePath);
		if(!data)
			return false;

		initGL(data);
		stbi_image_free(data);

		return true;
	}

	bool Texture::initFromFileAutoInterFormat(std::string_view filePath, bool convertLinear)
	{
		void* data = getDataAndPropsFromFile(filePath);
		if(!data)
			return false;
	
		/*** auto internal format ***/
		internal_format = -1;

		// sRGB는 밝은영역을 압축하기 위함이라 8비트 이상이 필요없음
		// From : https://registry.khronos.org/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
		// Todo: GL_SRGB == GL_SRGB8 ?
		// 		SNORM은 언제 쓰지?
		//		I, UI, F 가 무슨 의미가 있지?
		log::pure("texture %s loading auto with ", file_path.c_str());
		if( convertLinear ) {
			switch(src_nr_channels) {
				case 3: internal_format = GL_SRGB8; 		log::pure("GL_SRGB8"); break;
				case 4: internal_format = GL_SRGB8_ALPHA8; 	log::pure("GL_SRGB8"); break;
			}
		}
		else {
			if(src_bit_per_channel==8) {
				switch(src_nr_channels) {
				case 1: internal_format = GL_R8; 	log::pure("GL_R8"); break;
				case 2: internal_format = GL_RG8; 	log::pure("GL_RG8"); break;
				case 3: internal_format = GL_RGB8; 	log::pure("GL_RGB8"); break;
				case 4: internal_format = GL_RGBA8; log::pure("GL_RGBA8"); break;
				}
			}
			else if(src_bit_per_channel==16) {
				switch(src_nr_channels) {
				case 1: internal_format = GL_R16; 	log::pure("GL_R16"); break;
				case 2: internal_format = GL_RG16; 	log::pure("GL_RG16"); break;
				case 3: internal_format = GL_RGB16; log::pure("GL_RGB16"); break;
				case 4: internal_format = GL_RGBA16;log::pure("GL_RGBA16"); break;
				}
			}
			else if(src_bit_per_channel==32) {
				switch(src_nr_channels) {
				case 1: internal_format = GL_R32F; 	 log::pure("GL_R32F"); break;
				case 2: internal_format = GL_RG32F;  log::pure("GL_RG32F"); break;
				case 3: internal_format = GL_RGB32F; log::pure("GL_RGB32F"); break;
				case 4: internal_format = GL_RGBA32F;log::pure("GL_RGBA32F"); break;
				}
			}
		}

		initGL(data);
		stbi_image_free(data);

		return true;
	}
	

	
	void drawTexToQuad(const GLuint texId, float gamma) 
	{
		const Program& prog = AssetLib::get().tex_to_quad_prog;

		prog.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texId);
		prog.setUniform("tex", 0);
		prog.setUniform("gamma", gamma);

		AssetLib::get().screen_quad.drawGL();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void copyTexToTex(const GLuint srcTexId, Texture& dstTex) 
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
	void copyTexToTex(const Texture& srcTex, Texture& dstTex) 
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