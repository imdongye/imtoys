//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#include <limbrary/model_view/mesh.h>
#include <limbrary/gl_tools.h>
#include <limbrary/log.h>
#include <limbrary/gl_tools.h>

using namespace std;
using namespace glm;
using namespace lim;


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
	assert( poss.empty() == false ); // no verts in mesh

	if( vao!=0 ) {
		log::warn("recreate mesh buffer");
		deinitGL();
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	nr_verts = poss.size();
	nr_tris = tris.size();

	if( poss.size()>0 ){
		glGenBuffers(1, &buf_pos);
		glBindBuffer(GL_ARRAY_BUFFER, buf_pos);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*poss.size(), poss.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
	}
	if( nors.size()>0 ){
		glGenBuffers(1, &buf_nor);
		glBindBuffer(GL_ARRAY_BUFFER, buf_nor);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*nors.size(), nors.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
	}
	if( uvs.size()>0 ){
		glGenBuffers(1, &buf_uv);
		glBindBuffer(GL_ARRAY_BUFFER, buf_uv);
		glBufferData(GL_ARRAY_BUFFER, sizeof(uvs[0])*uvs.size(), uvs.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( cols.size()>0 ){
		glGenBuffers(1, &buf_color);
		glBindBuffer(GL_ARRAY_BUFFER, buf_color);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cols[0])*cols.size(), cols.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( tangents.size()>0 ){
		glGenBuffers(1, &buf_tangent);
		glBindBuffer(GL_ARRAY_BUFFER, buf_tangent);
		glBufferData(GL_ARRAY_BUFFER, sizeof(tangents[0])*tangents.size(), tangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( bitangents.size()>0 ){
		glGenBuffers(1, &buf_bitangent);
		glBindBuffer(GL_ARRAY_BUFFER, buf_bitangent);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bitangents[0])*bitangents.size(), bitangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}
	if( bone_infos.size()>0 ){
		glGenBuffers(1, &buf_bone_info);
		glBindBuffer(GL_ARRAY_BUFFER, buf_bone_info);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bone_infos[0])*bone_infos.size(), bone_infos.data(), GL_STATIC_DRAW);
		
		GLsizei stride = sizeof(VertBoneInfo);
		const void* weightOffset = (void *)(MAX_BONE_PER_VERT*sizeof(int));

		glEnableVertexAttribArray(6);
		glVertexAttribIPointer(6, MAX_BONE_PER_VERT, GL_INT, stride, 0);
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, MAX_BONE_PER_VERT, GL_FLOAT, GL_FALSE, stride, weightOffset);
	}
	if( tris.size()>0 ){
		glGenBuffers(1, &buf_tris);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_tris);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uvec3)*tris.size(), tris.data(), GL_STATIC_DRAW);
	}

	if( withClearMem ) {
		glFinish();
		clearMem();
	}
}
void Mesh::restorePosBuf()
{
	assert( buf_pos != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_pos);
	// glMap보다 조금 더 빠름, buffer usage dynamic_draw에 따른 차이 없음
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, poss.data());
}
void Mesh::restoreNorBuf()
{
	assert( buf_nor != 0 );
	glBindBuffer(GL_ARRAY_BUFFER, buf_nor);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3)*nr_verts, nors.data());
}
void Mesh::deinitGL()
{
	gl::safeDelBufs(&buf_pos);
	gl::safeDelBufs(&buf_nor);
	gl::safeDelBufs(&buf_uv);
	gl::safeDelBufs(&buf_color);
	gl::safeDelBufs(&buf_tangent);
	gl::safeDelBufs(&buf_bitangent);
	gl::safeDelBufs(&buf_bone_info);
	gl::safeDelBufs(&buf_tris);
	gl::safeDelVertArrs(&vao);
}
void Mesh::clearMem() {
	poss.clear(); 		poss.shrink_to_fit();
	nors.clear(); 		nors.shrink_to_fit();
	uvs.clear(); 		uvs.shrink_to_fit();
	cols.clear(); 		cols.shrink_to_fit();
	tangents.clear(); 	tangents.shrink_to_fit();
	bitangents.clear(); bitangents.shrink_to_fit();
	bone_infos.clear(); bone_infos.shrink_to_fit();
	tris.clear(); 		tris.shrink_to_fit();
}
void Mesh::bindGL() const {
	assert( vao != 0 && buf_tris != 0 );
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_tris);
}
void Mesh::drawGL() const {
	glDrawElements(GL_TRIANGLES, nr_tris*3, GL_UNSIGNED_INT, nullptr);
}
void Mesh::bindAndDrawGL() const {
	assert( vao != 0 && buf_tris != 0 );
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_tris);
	glDrawElements(GL_TRIANGLES, nr_tris*3, GL_UNSIGNED_INT, nullptr);
}
void Mesh::print() const
{
	log::pure("mesh: %s, verts %d, tris %d\n", name.c_str(), poss.size(), tris.size());
}