//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#include <limbrary/model_view/mesh.h>
#include <limbrary/gl_tools.h>
#include <limbrary/log.h>

using namespace std;
using namespace glm;
using namespace lim;


inline static void deleteGLBuf(GLuint& buf) {
	if( buf>0 ){
		glDeleteBuffers(1, &buf);
		buf = 0;
	}
}


Mesh::Mesh()
{
}
Mesh::Mesh(const Mesh& src)
	: name(src.name+"-cloned")
	, poss(src.poss)
	, nors(src.nors)
	, uvs(src.uvs)
	, cols(src.cols)
	, tangents(src.tangents)
	, bitangents(src.bitangents)
	, bone_infos(src.bone_infos)
	, tris(src.tris)
{
	initGL();
}
Mesh::~Mesh()
{
	deinitGL();
}
// upload VRAM
void Mesh::initGL(bool withClearMem)
{
	if( poss.size()==0 ){
		log::err("no verts in mesh\n\n");
		std::exit(-1);
		return;
	}
	deinitGL();

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &pos_buf);
	glBindBuffer(GL_ARRAY_BUFFER, pos_buf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(poss[0])*poss.size(), poss.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	if( nors.size()>0 ){
		glGenBuffers(1, &nor_buf);
		glBindBuffer(GL_ARRAY_BUFFER, nor_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(nors[0])*nors.size(), nors.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( uvs.size()>0 ){
		glGenBuffers(1, &uv_buf);
		glBindBuffer(GL_ARRAY_BUFFER, uv_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs[0])*uvs.size(), uvs.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( cols.size()>0 ){
		glGenBuffers(1, &color_buf);
		glBindBuffer(GL_ARRAY_BUFFER, color_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cols[0])*cols.size(), cols.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( tangents.size()>0 ){
		glGenBuffers(1, &tangent_buf);
		glBindBuffer(GL_ARRAY_BUFFER, tangent_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tangents[0])*tangents.size(), tangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( bitangents.size()>0 ){
		glGenBuffers(1, &bitangent_buf);
		glBindBuffer(GL_ARRAY_BUFFER, bitangent_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bitangents[0])*bitangents.size(), bitangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( tris.size()>0 ){
		glGenBuffers(1, &element_buf);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uvec3)*tris.size(), tris.data(), GL_STATIC_DRAW);
	}



	if( bone_infos.size()>0 ){
		// override vbo in main vao
		glBindVertexArray(vao);
		glGenBuffers(1, &skinned_pos_buf);
		glBindBuffer(GL_ARRAY_BUFFER, skinned_pos_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(poss[0])*poss.size(), poss.data(), GL_DYNAMIC_COPY);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		if( nors.size()>0 ){
			glGenBuffers(1, &skinned_nor_buf);
			glBindBuffer(GL_ARRAY_BUFFER, skinned_nor_buf);
			glBufferData(GL_ARRAY_BUFFER, sizeof(nors[0])*nors.size(), nors.data(), GL_DYNAMIC_COPY);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		}


		// for baking skkend mesh
		glGenVertexArrays(1, &skinning_vao);
		glBindVertexArray(skinning_vao);
		
		glBindBuffer(GL_ARRAY_BUFFER, pos_buf);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glBindBuffer(GL_ARRAY_BUFFER, nor_buf);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &bone_infos_buf);
		glBindBuffer(GL_ARRAY_BUFFER, bone_infos_buf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bone_infos[0])*bone_infos.size(), bone_infos.data(), GL_STATIC_DRAW);
		GLsizei stride = sizeof(VertBoneInfo);
		const void* weightOffset = (void *)(MAX_BONE_PER_VERT*sizeof(int));
		glEnableVertexAttribArray(2);
		glVertexAttribIPointer(2, MAX_BONE_PER_VERT, GL_INT, stride, 0);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, MAX_BONE_PER_VERT, GL_FLOAT, GL_FALSE, stride, weightOffset);
	}

	if( withClearMem )
		clearMem();
}
void Mesh::deinitGL()
{
	gl::safeDelBufs(&pos_buf);
	gl::safeDelBufs(&nor_buf);
	gl::safeDelBufs(&skinned_pos_buf);
	gl::safeDelBufs(&skinned_nor_buf);

	gl::safeDelBufs(&uv_buf);
	gl::safeDelBufs(&tangent_buf);
	gl::safeDelBufs(&bitangent_buf);
	gl::safeDelBufs(&color_buf);
	gl::safeDelBufs(&element_buf);
	gl::safeDelVertArrs(&vao);

	gl::safeDelBufs(&bone_infos_buf);
	gl::safeDelVertArrs(&skinning_vao);
}
void Mesh::clearMem() {
	poss.clear(); poss.shrink_to_fit();
	nors.clear(); nors.shrink_to_fit();
	uvs.clear(); uvs.shrink_to_fit();
	cols.clear(); cols.shrink_to_fit();
	tangents.clear(); tangents.shrink_to_fit();
	bitangents.clear(); bitangents.shrink_to_fit();
	bone_infos.clear(); bone_infos.shrink_to_fit();
	tris.clear(); tris.shrink_to_fit();
}
void Mesh::bindGL() const {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
}
void Mesh::drawGL() const {
	glDrawElements(GL_TRIANGLES, tris.size()*3, GL_UNSIGNED_INT, nullptr);
}
void Mesh::bindAndDrawGL() const {
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
	glDrawElements(GL_TRIANGLES, tris.size()*3, GL_UNSIGNED_INT, nullptr);
}
void Mesh::print() const
{
	log::pure("mesh: %s, verts %d, tris %d\n", name.c_str(), poss.size(), tris.size());
}