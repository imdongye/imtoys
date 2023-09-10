#include <limbrary/asset_lib.h>
#include <limbrary/program.h>
#include <limbrary/log.h>

namespace lim
{
	AssetLib::AssetLib()
	{
		log::pure("init AssetLib\n");

		// Array for full-screen quad
		GLfloat vertices[] ={
			-1.0f,1.0f,0.0f,  1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, // top right
			-1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, -1.0f,-1.0f,0.0f, // bottom left
		};

		// Set up the buffers
		glGenBuffers(1, &quad_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Set up the vertex array object
		glGenVertexArrays(1, &quad_vao);
		glBindVertexArray(quad_vao);

		glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);  // Vertex position

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0); 

		log::pure("load quad vao to vram\n");


		tex_to_quad_prog = new Program("texToQuad");
		tex_to_quad_prog->attatch("quad.vs").attatch("tex_to_quad.fs").link();

		gray_to_quad_prog = new Program("grayToQuad");
		gray_to_quad_prog->attatch("quad.vs").attatch("gray_to_quad.fs").link();
	
		depth_prog = new Program("depth");
		depth_prog->attatch("mvp.vs").attatch("depth.fs").link();

		red_prog = new Program("red");
		red_prog->attatch("quad.vs").attatch("red.fs").link();
	}
	AssetLib::~AssetLib()
	{
		log::pure("delete AssetLib\n");

		delete tex_to_quad_prog;
		delete gray_to_quad_prog;
		delete depth_prog;
		delete red_prog;
		glDeleteVertexArrays(1, &quad_vao); quad_vao=0;
		glDeleteBuffers(1, &quad_vbo); quad_vbo=0;
	}
	AssetLib& AssetLib::get()
	{
		if( !instance ) {
			instance = new AssetLib();
		}
		return *instance;
	}
	void AssetLib::destroy()
	{
		if( instance )
			delete instance;
		instance = nullptr;
	}
}