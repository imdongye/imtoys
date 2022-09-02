//
//	2022-08-24 / im dong ye
//
//	TODO list:
//
#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "program.h"
#include "imgui_modules.h"

namespace lim
{
	class Framebuffer
	{
	private:
		// BSS 메모리에 저장되어 0으로 초기화된다
		static Program* toScrProgram;
	public:
		GLuint width=0, height=0;
		GLuint FBO, colorTex, RBO, quadVAO;
	public:
		Framebuffer(): FBO(0)
		{
			if( toScrProgram == nullptr )
			{
				toScrProgram = new Program("toScrProgram");
				toScrProgram->attatch("shader/fb_to_scr.vs").attatch("shader/fb_to_scr.fs").link();
			}

			GLuint loc = glGetUniformLocation(toScrProgram->use(), "screenTex");
			glUniform1i(loc, 0);

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
			glDeleteFramebuffers(1, &FBO);
			glDeleteTextures(1, &colorTex);
			glDeleteRenderbuffers(1, &RBO);
			FBO = colorTex = RBO = 0;
		}
		void resize(GLuint _width, GLuint _height)
		{
			width = _width; height = _height;
			reset();
		}
		void reset()
		{
			if( FBO != 0 )
				clear();

			glGenTextures(1, &colorTex);
			glBindTexture(GL_TEXTURE_2D, colorTex);
			// glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			// why 2의 승수가 아니여도 돼나?
			//glTexStorage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, width); // storage는 텍스쳐 크기변경불가능
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Create the depth buffer (stencil)
			// render buffer은 셰이더에서 접근할수없는 텍스쳐, 속도면에서 장점
			glGenRenderbuffers(1, &RBO);
			glBindRenderbuffer(GL_RENDERBUFFER, RBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);


			glGenFramebuffers(1, &FBO);
			glBindFramebuffer(GL_FRAMEBUFFER, FBO);
			// Bind the texture to the FBO
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);
			// Bind the depth buffer to the FBO
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);

			// Set the targets for the fragment output variables
			//GLenum drawBuffers[] ={GL_COLOR_ATTACHMENT0};
			//glDrawBuffers(1, drawBuffers);

			// error check
			if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) fprintf(stderr, "FBO Error %d %d\n", width, height);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		void copyToBackBuf()
		{
			if( FBO==0 )
				return;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, width, height);
			glDisable(GL_DEPTH_TEST);

			glClearColor(1, 1, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			toScrProgram->use();
			glBindVertexArray(quadVAO);
			glBindTexture(GL_TEXTURE_2D, colorTex);	// use the color attachment texture as the texture of the quad plane
			glActiveTexture(GL_TEXTURE0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glBindVertexArray(0);
		}
	};
	Program* Framebuffer::toScrProgram = nullptr;
} // ! namespace lim

#endif