#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/log.h>

namespace lim
{
	AssetLib::AssetLib()
	{
		// Array for full-screen quad
		GLfloat vertices[] ={
			-1.0f,1.0f,0.0f,  1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, // top right
			-1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, -1.0f,-1.0f,0.0f, // bottom left
		};

		// Set up the buffers
		GLuint vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, 6 * 3*sizeof(GLfloat), vertices, GL_STATIC_DRAW);

		// Set up the vertex array object
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);

		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);  // Vertex position

		log::info("load quad vao to vram\n");


		tex_to_quad_prog = new Program("texToQuad");
		tex_to_quad_prog->attatch("tex_to_quad.vs").attatch("tex_to_quad.fs").link();


		gray_to_quad_prog = new Program("grayToQuad");
		gray_to_quad_prog->attatch("tex_to_quad.vs").attatch("gray_to_quad.fs").link();
	}
	AssetLib::~AssetLib()
	{
		delete tex_to_quad_prog;
		glDeleteVertexArrays(1, &quad_vao); quad_vao=0;
	}
	AssetLib& AssetLib::get()
	{
		if( !instance ) {
			instance = new AssetLib();
		}
		return *instance;
	}
	void AssetLib::reload()
	{
		if( !instance )delete instance;
		instance = new AssetLib();
	}
}