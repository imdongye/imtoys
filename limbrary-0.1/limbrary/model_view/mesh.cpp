//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//	todo :
//	1. bumpmap normalmap확인

#include <limbrary/model_view/mesh.h>
#include <limbrary/log.h>

using namespace std;
using namespace glm;

namespace {
	inline void deleteGLBuf(GLuint& buf) {
		if( buf>0 ){
			glDeleteBuffers(1, &buf);
			buf = 0;
		}
	}
}

namespace lim
{
	Mesh::Mesh()
	{
	}
	Mesh::Mesh(const Mesh& src)
	{
		name = src.name+"-clone";
		poss = src.poss;
		nors = src.nors;
		uvs = src.uvs;
		cols = src.cols;
		tangents = src.tangents;
		bitangents = src.bitangents;
		bone_ids = src.bone_ids;
		bending_factors = src.bending_factors;
		tris = src.tris;
		initGL();
	}
	Mesh::Mesh(Mesh&& src) noexcept
	{
		*this = std::move(src);
	}
	Mesh& Mesh::operator=(Mesh&& src) noexcept
	{
		if( this != &src ) {
			deinitGL();

			name 			= move(src.name);
			poss 			= move(src.poss);
			nors 			= move(src.nors);
			uvs 			= move(src.uvs);
			cols 			= move(src.cols);
			tangents 		= move(src.tangents);
			bitangents 		= move(src.bitangents);
			bone_ids 		= move(src.bone_ids);
			bending_factors = move(src.bending_factors);
			tris			= move(src.tris);

			pos_buf = src.pos_buf;
			nor_buf = src.nor_buf;
			uv_buf  = src.uv_buf ;
			color_buf = src.color_buf;
			tangent_buf = src.tangent_buf;
			bitangent_buf = src.bitangent_buf;
			bone_id_buf = src.bone_id_buf;
			bending_factor_buf = src.bending_factor_buf;
			element_buf = src.element_buf;
			vert_array = src.vert_array;
			// 필요한가?
			src.pos_buf = 0;
			src.nor_buf = 0;
			src.uv_buf  = 0;
			src.color_buf = 0;
			src.tangent_buf = 0;
			src.bitangent_buf = 0;
			src.bone_id_buf = 0;
			src.bending_factor_buf = 0;
			src.element_buf = 0;
		}
		return *this;
	}
	Mesh::~Mesh() noexcept
	{
		deinitGL();
	}
	// upload VRAM
	void Mesh::initGL()
	{
		if( poss.size()==0 ){
			log::err("no verts in mesh\n\n");
			std::exit(-1);
			return;
		}
		deinitGL();

		glGenVertexArrays(1, &vert_array);
		glBindVertexArray(vert_array);

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
		if( bone_ids.size()>0 ){
			glGenBuffers(1, &bone_id_buf);
			glBindBuffer(GL_ARRAY_BUFFER, bone_id_buf);
			glBufferData(GL_ARRAY_BUFFER, sizeof(bone_ids[0])*bone_ids.size(), bone_ids.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(6);
			glVertexAttribPointer(6, MAX_BONE_INFLUENCE, GL_INT, GL_FALSE, 0, 0);
		}
		if( bending_factors.size()>0 ){
			glGenBuffers(1, &bending_factor_buf);
			glBindBuffer(GL_ARRAY_BUFFER, bending_factor_buf);
			glBufferData(GL_ARRAY_BUFFER, sizeof(bending_factors[0])*bending_factors.size(), bending_factors.data(), GL_STATIC_DRAW);
			glEnableVertexAttribArray(7);
			glVertexAttribPointer(7, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, 0, 0);
		}
		if( tris.size()>0 ){
			glGenBuffers(1, &element_buf);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uvec3)*tris.size(), tris.data(), GL_STATIC_DRAW);
		}
	}
	void Mesh::deinitGL()
	{
		deleteGLBuf(pos_buf); 				//poss.clear();
		deleteGLBuf(nor_buf);				//nors.clear();
		deleteGLBuf(uv_buf);				//uvs.clear();
		deleteGLBuf(tangent_buf);			//tangents.clear();
		deleteGLBuf(bitangent_buf);			//bitangents.clear();
		deleteGLBuf(color_buf);				//cols.clear();
		deleteGLBuf(bone_id_buf);			//bone_ids.clear();
		deleteGLBuf(bending_factor_buf);	//bending_factors.clear();
		deleteGLBuf(element_buf);			//tris.clear();
		if( vert_array>0 ){
			glDeleteVertexArrays(1, &vert_array);
			vert_array = 0;
		}
	}
	void Mesh::drawGL() const
	{
		glBindVertexArray(vert_array);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buf);
		glDrawElements(GL_TRIANGLES, tris.size()*3, GL_UNSIGNED_INT, 0);
	}
	void Mesh::print() const
	{
		log::pure("mesh: %s, verts %d, tris %d\n", name.c_str(), poss.size(), tris.size());
	}
}