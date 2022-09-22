//
//	2022-08-24 / im dong ye
//
//	처음엔 imgui로 보여주기위한 renderTex를 생성하기위해 만들었다.
//	mass를 적용하기위해 intermediateFbo를 만들었고 msFbo에 draw하고
//	intermediateFbo에 blit 해주는 과정을 밖에서 해줘야한다.
// 
//	MSAA frame buffer
//	from : https://learnopengl.com/Advanced-OpenGL/Anti-Aliasing
//
//	todo:
//	1. 부모소멸자 호출안하는 방법이 있나? framebuffer::clear두번호출
//	2. create 가상함수 재활용
//

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

namespace lim
{
	class Framebuffer
	{
	protected:
		/* c++17 non-const static data member can initialize in declaration with inline keyword*/
		inline static Program toScrProgram = Program("toScr");
		inline static GLuint quadVAO = 0;
	public:
		GLuint fbo, colorTex;
	public:
		glm::vec4 clearColor={0,0,1,1};
		GLuint width=0, height=0;
	public:
		Framebuffer()
		{
			fbo=colorTex=0;
			width=height=0;
			if( toScrProgram.ID==0 ) {
				toScrProgram.attatch("fb_to_scr.vs").attatch("fb_to_scr.fs").link();
				GLuint loc = glGetUniformLocation(toScrProgram.use(), "screenTex");
				glUniform1i(loc, 0);
			}
			if( quadVAO == 0 ) {
				// Array for full-screen quad
				GLfloat verts[] ={
					-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
					-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f
				};
				GLfloat tc[] ={
					0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
					0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
				};

				// Set up the buffers
				unsigned int handle[2];
				glGenBuffers(2, handle);
				glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
				glBufferData(GL_ARRAY_BUFFER, 6 * 3 * sizeof(float), verts, GL_STATIC_DRAW);
				glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
				glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), tc, GL_STATIC_DRAW);

				// Set up the vertex array object
				glGenVertexArrays(1, &quadVAO);
				glBindVertexArray(quadVAO);
				glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
				glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(0);  // Vertex position
				glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
				glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(1);  // Texture coordinates
				glBindVertexArray(0);
			}
		}
		Framebuffer(GLuint _width, GLuint _height=0):Framebuffer()
		{
			resize(_width, _height);
			create();
		}
		virtual ~Framebuffer()
		{
			clear();
			if( quadVAO!=0 ) {
				glDeleteVertexArrays(1, &quadVAO);
				quadVAO=0;
			}
			// static pointer delete가 필요한가?
		}
	public:
		bool renderable()
		{
			return fbo!=0;
		}
		void resize(GLuint _width, GLuint _height=0)
		{
			width = _width;
			height = (_height==0)?_width:_height;
			create();
		}
		void copyToBackBuf()
		{
			textureToBackBuf(getRenderedTex());
		}
	public:
		virtual void create()
		{
			createColorTex(colorTex);

			/* create FBO */
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("color FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual GLuint getRenderedTex()
		{
			return colorTex;
		}
		/* 오버라이딩하고 첫줄에 부모 가상함수를 꼭 호출해줘야함. */
		virtual void clear()
		{
			if( colorTex ) glDeleteTextures(1, &colorTex); colorTex=0;
			if( fbo ) glDeleteFramebuffers(1, &fbo); fbo=0;
		}
		virtual void bind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glViewport(0, 0, width, height);
			glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
			glClear(GL_COLOR_BUFFER_BIT);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);
		}
		virtual void unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	protected:
		inline void createColorTex(GLuint& id)
		{
			// glTexStorage2D 텍스쳐 크기 고정
			// glTexImage2D 텍스쳐크기 변경가능 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			setTexParam(GL_LINEAR);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	protected:
		static inline void setTexParam(GLuint minFilter = GL_LINEAR, GLuint warp = GL_CLAMP_TO_EDGE)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, warp);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, warp);
		}
		void textureToBackBuf(GLuint texID, bool toSRGB = false)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, width, height);
			if( toSRGB ) glEnable(GL_FRAMEBUFFER_SRGB);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_MULTISAMPLE);

			glClearColor(1, 1, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			toScrProgram.use();
			glBindVertexArray(quadVAO);
			// use the color attachment texture as the texture of the quad plane
			glBindTexture(GL_TEXTURE_2D, texID);
			glActiveTexture(GL_TEXTURE0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindVertexArray(0);
		}
	};


	class TxFramebuffer: public Framebuffer
	{
	public:
		GLuint depth;
	public:
		TxFramebuffer(): Framebuffer()
		{
			depth=0;
		}
		virtual ~TxFramebuffer()
		{
			clear();
		}
	public:
		virtual void clear() final
		{
			Framebuffer::clear();
			if( depth ) glDeleteRenderbuffers(1, &depth); depth=0;
		}
		virtual void create() final
		{
			createColorTex(colorTex);
			createDepthTex(depth);

			/* create FBO */
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
			//glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth, 0);
			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("tx FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			// Set the targets for the fragment output variables 필요한가?
			//GLenum drawBuffers[] ={GL_COLOR_ATTACHMENT0};
			//glDrawBuffers(1, drawBuffers);
		}
		virtual void bind() final
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	protected:
		inline void createDepthTex(GLuint& id)
		{
			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};

	class RbFramebuffer: public Framebuffer
	{
	protected:
		GLuint rbo;
	public:
		RbFramebuffer()
		{
			rbo=0;
		}
		virtual ~RbFramebuffer()
		{
			clear();
		}
	protected:
		virtual void clear()
		{
			Framebuffer::clear();
			if( rbo ) glDeleteRenderbuffers(1, &rbo); rbo=0;
		}
		virtual void create()
		{
			/* create FBO */
			createColorTex(colorTex);
			createRBO(rbo);
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("rbo FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual void bind()
		{
			Framebuffer::bind();
			glEnable(GL_DEPTH_TEST);
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	protected:
		inline void createRBO(GLuint& id)
		{
			// Create the depth buffer (stencil)
			// render buffer은 셰이더에서 접근할수없는 텍스쳐, 속도면에서 장점
			glGenRenderbuffers(1, &id);
			glBindRenderbuffer(GL_RENDERBUFFER, id);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
	};

	class MsFramebuffer: public RbFramebuffer
	{
	private:
		/* colorTex와 rbo를 multisampliing 모드로 생성 */
		const int samples = 16;
		GLuint intermediateFBO, screenTex;
	public:
		MsFramebuffer()
		{
			intermediateFBO=screenTex=0;
		}
		virtual ~MsFramebuffer()
		{
			clear();
		}
	public:
		virtual void create() final
		{
			clear();
			/* multisampled FBO 생성 */
			createMsColorTex(colorTex);
			createMsRBO(rbo);
			glGenFramebuffers(1, &fbo); // multisampledFBO
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTex, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("multisampled FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			/* for pass imgui image and post processing */
			createColorTex(screenTex);
			glGenFramebuffers(1, &intermediateFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTex, 0);
			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				Logger::get().log("intermediate FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		virtual GLuint getRenderedTex() final
		{
			return screenTex;
		}
		virtual void clear() final
		{
			RbFramebuffer::clear();
			if( screenTex ) glDeleteTextures(1, &screenTex); screenTex=0;
			if( intermediateFBO ) glDeleteFramebuffers(1, &intermediateFBO); intermediateFBO=0;
		}
		virtual void bind() final
		{
			RbFramebuffer::bind();
		}
		virtual void unbind() final
		{
			/* msaa framebuffer은 일반 texture을 가진 frame버퍼에 blit 복사 해야함 */
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height
							  , GL_COLOR_BUFFER_BIT, GL_NEAREST);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	private:
		inline void createMsColorTex(GLuint& id)
		{
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, id);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
			setTexParam(GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
		inline void createMsRBO(GLuint& id)
		{
			glGenRenderbuffers(1, &id);
			glBindRenderbuffer(GL_RENDERBUFFER, id);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		}
	};
} // ! namespace lim

#endif