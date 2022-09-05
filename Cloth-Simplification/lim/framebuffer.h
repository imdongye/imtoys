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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "limclude.h"

namespace lim
{
	class Framebuffer
	{
	public:
		static Program* toScrProgram;
		static GLuint quadVAO;

		GLuint width=0, height=0;
		const int samples = 8;
		GLuint multisampledFBO, colorTex, rbo;
		GLuint intermediateFBO, screenTex;
	public:
		Framebuffer()
		{
			multisampledFBO=colorTex=rbo=0;
			intermediateFBO=screenTex=0;
			width=height=0;

			if( toScrProgram == 0 )
			{
				toScrProgram = new Program("toScrProgram");
				toScrProgram->attatch("shader/fb_to_scr.vs").attatch("shader/fb_to_scr.fs").link();
				GLuint loc = glGetUniformLocation(toScrProgram->use(), "screenTex");
				glUniform1i(loc, 0);
			}
			if( quadVAO == 0 )
			{
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
		Framebuffer(GLuint _width, GLuint _height): Framebuffer()
		{
			resize(_width, _height);
		}
		~Framebuffer()
		{
			clear();
			if( quadVAO!=0 )
			{
				glDeleteVertexArrays(1, &quadVAO);
				quadVAO=0;
			}
			// static pointer delete가 필요한가?
		}
		void clear()
		{
			if( colorTex ) glDeleteTextures(1, &colorTex); colorTex=0;
			if( rbo ) glDeleteRenderbuffers(1, &rbo); rbo=0;
			if( multisampledFBO ) glDeleteFramebuffers(1, &multisampledFBO); multisampledFBO=0;

			if( screenTex ) glDeleteTextures(1, &screenTex); screenTex=0;
			if( intermediateFBO ) glDeleteFramebuffers(1, &intermediateFBO); intermediateFBO=0;
		}
		void resize(GLuint _width, GLuint _height)
		{
			width = _width; height = _height;
			create();
		}
		void create()
		{
			clear();
			glGenFramebuffers(1, &multisampledFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, multisampledFBO);

			// glTexStorage2D 텍스쳐 크기 고정
			// glTexImage2D 텍스쳐크기 변경가능 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			// glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, MSAA

			const int samples = 8;
			glGenTextures(1, &colorTex);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTex);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, width, height, GL_TRUE);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
			// Bind the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTex, 0);


			// Create the depth buffer (stencil)
			// render buffer은 셰이더에서 접근할수없는 텍스쳐, 속도면에서 장점
			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			// Bind the depth buffer to the FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

			// Set the targets for the fragment output variables
			//GLenum drawBuffers[] ={GL_COLOR_ATTACHMENT0};
			//glDrawBuffers(1, drawBuffers);

			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				fprintf(stderr, "FBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);


			/* for pass imgui image and post processing */
			glGenFramebuffers(1, &intermediateFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);

			glGenTextures(1, &screenTex);
			glBindTexture(GL_TEXTURE_2D, screenTex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTex, 0);

			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE )
				fprintf(stderr, "postProcessingFBO Error %d %d\n", width, height);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		void copyToBackBuf()
		{
			if( multisampledFBO==0 )
				return;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_FRAMEBUFFER_SRGB);
			glViewport(0, 0, width, height);
			glDisable(GL_DEPTH_TEST);

			glClearColor(1, 1, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			toScrProgram->use();
			glBindVertexArray(quadVAO);
			glBindTexture(GL_TEXTURE_2D, screenTex);	// use the color attachment texture as the texture of the quad plane
			glActiveTexture(GL_TEXTURE0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindVertexArray(0);
		}
	};
	// BSS 메모리에 저장되어 0으로 초기화되는거 아닌가??
	Program* Framebuffer::toScrProgram = 0;
	GLuint Framebuffer::quadVAO = 0;
} // ! namespace lim

#endif