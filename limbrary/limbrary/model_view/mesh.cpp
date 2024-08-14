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
Mesh::Mesh(Mesh&& src)
	: name(move(src.name))
	, poss(move(src.poss))
	, nors(move(src.nors))
	, uvs(move(src.uvs))
	, cols(move(src.cols))
	, tangents(move(src.tangents))
	, bitangents(move(src.bitangents))
	, bone_infos(move(src.bone_infos))
	, tris(move(src.tris))
	, nr_verts(src.nr_verts)
	, nr_tris(src.nr_tris)
	, buf_poss(src.buf_poss), buf_nors(src.buf_nors), buf_uvs(src.buf_uvs)
	, buf_colors(src.buf_colors), buf_tangents(src.buf_tangents), buf_bitangents(src.buf_bitangents)
	, buf_bone_infos(src.buf_bone_infos), buf_tris(src.buf_tris), vao(src.vao)
{
	src.buf_poss = 0;
	src.buf_nors = 0;
	src.buf_uvs = 0;
	src.buf_colors = 0;
	src.buf_tangents = 0;
	src.buf_bitangents = 0;
	src.buf_bone_infos = 0;
	src.buf_tris = 0;
	src.vao = 0;
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
	, nr_verts(poss.size())
	, nr_tris(tris.size())
{
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
		log::warn("recreate mesh buffer\n");
		deinitGL();
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	nr_verts = poss.size();
	nr_tris = tris.size();

	// now no assert for nr_verts != nr_attribs
	// because SoftBody reinit in there initGL
	if( poss.size()>0 ){
		size_t elem_size = sizeof(vec3);
		glGenBuffers(1, &buf_poss);
		glBindBuffer(GL_ARRAY_BUFFER, buf_poss);
		glBufferData(GL_ARRAY_BUFFER, elem_size*poss.size(), poss.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( nors.size()>0 ){
		size_t elem_size = sizeof(vec3);
		glGenBuffers(1, &buf_nors);
		glBindBuffer(GL_ARRAY_BUFFER, buf_nors);
		glBufferData(GL_ARRAY_BUFFER, elem_size*nors.size(), nors.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( uvs.size()>0 ){
		size_t elem_size = sizeof(vec2);
		glGenBuffers(1, &buf_uvs);
		glBindBuffer(GL_ARRAY_BUFFER, buf_uvs);
		glBufferData(GL_ARRAY_BUFFER, elem_size*uvs.size(), uvs.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( cols.size()>0 ){
		size_t elem_size = sizeof(vec3);
		glGenBuffers(1, &buf_colors);
		glBindBuffer(GL_ARRAY_BUFFER, buf_colors);
		glBufferData(GL_ARRAY_BUFFER, elem_size*cols.size(), cols.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( tangents.size()>0 ){
		size_t elem_size = sizeof(vec3);
		glGenBuffers(1, &buf_tangents);
		glBindBuffer(GL_ARRAY_BUFFER, buf_tangents);
		glBufferData(GL_ARRAY_BUFFER, elem_size*tangents.size(), tangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( bitangents.size()>0 ){
		size_t elem_size = sizeof(vec3);
		glGenBuffers(1, &buf_bitangents);
		glBindBuffer(GL_ARRAY_BUFFER, buf_bitangents);
		glBufferData(GL_ARRAY_BUFFER, elem_size*bitangents.size(), bitangents.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, elem_size, 0);
	}
	if( bone_infos.size()>0 ){
		size_t elem_size = sizeof(VertBoneInfo);
		glGenBuffers(1, &buf_bone_infos);
		glBindBuffer(GL_ARRAY_BUFFER, buf_bone_infos);
		glBufferData(GL_ARRAY_BUFFER, elem_size*bone_infos.size(), bone_infos.data(), GL_STATIC_DRAW);
		
		GLsizei stride = sizeof(VertBoneInfo);
		const void* weightOffset = (void *)(MAX_BONE_PER_VERT*sizeof(int));

		glEnableVertexAttribArray(6);
		glVertexAttribIPointer(6, MAX_BONE_PER_VERT, GL_INT, elem_size, 0);
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, MAX_BONE_PER_VERT, GL_FLOAT, GL_FALSE, elem_size, weightOffset);
	}
	if( tris.size()>0 ){
		size_t elem_size = sizeof(uvec3);
		glGenBuffers(1, &buf_tris);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf_tris);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, elem_size*tris.size(), tris.data(), GL_STATIC_DRAW);
	}

	if( withClearMem ) {
		glFinish();
		clearMem();
	}
}
void Mesh::deinitGL()
{
	gl::safeDelBufs(&buf_poss);
	gl::safeDelBufs(&buf_nors);
	gl::safeDelBufs(&buf_uvs);
	gl::safeDelBufs(&buf_colors);
	gl::safeDelBufs(&buf_tangents);
	gl::safeDelBufs(&buf_bitangents);
	gl::safeDelBufs(&buf_bone_infos);
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