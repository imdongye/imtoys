#include <limbrary/model_view/renderer.h>


namespace lim
{
    void render( const Framebuffer& fb, 
                 const Program& prog,
                 const Camera& cam,
                 const Model& md, 
                 const Light& lit )
    {
        fb.bind();
        prog.use();
        prog.setUniform("cameraPos", cam.position);
        prog.setUniform("projMat", cam.proj_mat);
        prog.setUniform("viewMat", cam.view_mat);
        prog.setUniform("modelMat", md.model_mat);

        prog.setUniform("lightDir", lit.direction);
        prog.setUniform("lightColor", lit.color);
        prog.setUniform("lightInt", lit.intensity);
        prog.setUniform("lightPos", lit.position);


        for( Mesh* pMesh : md.meshes)
        {
            const Mesh& mesh = *pMesh;

            glBindVertexArray(mesh.VAO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
            glDrawElements(mesh.draw_mode, static_cast<GLuint>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
        }

        fb.unbind();
    }
}
