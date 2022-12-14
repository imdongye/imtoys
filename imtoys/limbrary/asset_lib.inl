//
//	2022-12-30 / im dong ye
//	
//	inl: header only에서 circular dependency를 피하기 위한 방법
//

#ifndef ASSET_LIB_INL
#define ASSET_LIB_INL

lim::AssetLib::AssetLib()
{
	// Array for full-screen quad
	GLfloat verts[] ={
		-1.0f,1.0f,0.0f,  1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, // top right
		-1.0f,1.0f,0.0f,  1.0f,-1.0f,0.0f, -1.0f,-1.0f,0.0f, // bottom left
	};

	// Set up the buffers
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 6 * 3*sizeof(GLfloat), verts, GL_STATIC_DRAW);

	// Set up the vertex array object
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);  // Vertex position


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	Logger::get()<<"load quad vao to vram\n";


	to_quad_prog = new Program("toQuad");
	to_quad_prog->attatch("tex_to_quad.vs").attatch("tex_to_quad.fs").link();
}
lim::AssetLib::~AssetLib()
{
	delete to_quad_prog;
	glDeleteVertexArrays(1, &quad_vao); quad_vao=0;
}

#endif