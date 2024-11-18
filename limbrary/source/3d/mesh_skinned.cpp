#include <limbrary/3d/mesh_skinned.h>
#include <limbrary/tools/gl.h>

using namespace lim;
using namespace glm;

MeshSkinned::MeshSkinned(const Mesh& _src)
    : Mesh(), src(_src)
{
    assert(src.vao!=0);
    assert(src.poss.empty()==false);
    assert(src.nors.empty()==false);

    nr_verts = src.nr_verts;
    nr_tris = src.nr_tris;

    glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

    glGenBuffers(1, &buf_poss);
    glBindBuffer(GL_ARRAY_BUFFER, buf_poss);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*poss.size(), poss.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);

    glGenBuffers(1, &buf_nors);
    glBindBuffer(GL_ARRAY_BUFFER, buf_nors);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3)*nors.size(), nors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);

    if( buf_uvs>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_uvs);
        glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);
    }
    if( buf_colors>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_colors);
        glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    }
    if( buf_tangents>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_tangents);
        glEnableVertexAttribArray(4);
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    }
    if( buf_bitangents>0 ) {
        glBindBuffer(GL_ARRAY_BUFFER, buf_bitangents);
        glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), 0);
    }

    // important!! 
    // buf_tris id is copied but not own
    // so we do not delete when deconstruct
    buf_tris = src.buf_tris;
}

MeshSkinned::~MeshSkinned()
{
    // delete buf_poss, buf_nors, vao in ~Mesh

    buf_tris = 0; // no delete
}