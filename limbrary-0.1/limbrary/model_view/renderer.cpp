#include <limbrary/model_view/renderer.h>
#include <limbrary/asset_lib.h>
#include <stack>

using namespace std;

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

            glBindVertexArray(mesh.vert_array);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.element_buf);
            glDrawElements(GL_TRIANGLES, (GLsizei)(mesh.tris.size()*3), GL_UNSIGNED_INT, 0);
        }

        fb.unbind();
    }

    void render( const Framebuffer& fb,
                 const Camera& cam,
                 const Scene& scn )
    {
        stack<Model::Node*> nodeStack;
        const Program& depthProg = *AssetLib::get().depth_prog;

        for( Light* pLit : scn.lights) {
            const Light& lit = *pLit;

            if( lit.shadow_enabled ) 
            {
                lit.map_Shadow.bind();
                depthProg.use();

                depthProg.setUniform("viewMat", lit.view_mat);
                depthProg.setUniform("projMat", lit.proj_mat);

                for( Model* pMd : scn.models ) {
                    const Model& md = *pMd;
                    depthProg.setUniform("modelMat", md.model_mat);

                    for( Mesh* pMs : md.meshes ) {
                        const Mesh& ms = *pMs;
                        glBindVertexArray(ms.vert_array);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms.element_buf);
                        glDrawElements(GL_TRIANGLES, (GLsizei)(ms.tris.size()*3), GL_UNSIGNED_INT, 0);
                    }
                }

                lit.map_Shadow.unbind();
            }
        }

        fb.bind();

        Material* curMat = nullptr;
        Material* nextMat = nullptr;
        Program* curProg = nullptr;
        Program* nextProg = nullptr;
        for( Model* pMd : scn.models ) {
            const Model& md = *pMd;
            nextMat = md.default_mat;
            nextProg = md.default_mat->prog;

            nodeStack.push(md.root);
            while( nodeStack.size()>0 ) {
                const Model::Node& nd = *nodeStack.top();
                nodeStack.pop();
                for( Model::Node* child : nd.childs ) {
                    nodeStack.push(child);
                }

                for( Mesh* pMs : nd.meshes ) {
                    const Mesh& ms = *pMs;
                    const Program& prog = *nextProg;
                    int activeSlot = 0;
                    nextMat = ms.material;
                    nextProg = nextMat->prog;

                    if( nextProg && curProg != nextProg ) {
                        curProg = nextProg;
                        prog.use();
                        prog.setUniform("cameraPos", cam.position);
                        prog.setUniform("projMat", cam.proj_mat);
                        prog.setUniform("viewMat", cam.view_mat);
                        prog.setUniform("modelMat", md.model_mat); // todo

                        /* Todo: 지금은 라이트 하나만 */
                        for( Light* pLit : scn.lights ) {
                            const Light& lit = *pLit;
                            prog.setUniform("lightDir", lit.direction);
                            prog.setUniform("lightColor", lit.color);
                            prog.setUniform("lightInt", lit.intensity);
                            prog.setUniform("lightPos", lit.position);
                            prog.setUniform("shadowEnabled", (lit.shadow_enabled)?1:0);
                            if( lit.shadow_enabled ) {
                                prog.setUniform("shadowVP", lit.vp_mat);
                                glActiveTexture(GL_TEXTURE0 + activeSlot);
                                glBindTexture(GL_TEXTURE_2D, lit.map_Shadow.color_tex);
                                prog.setUniform("map_Shadow", activeSlot++);
                            }
                        }
                    }

                    if( nextMat && curMat != nextMat ) {
                        const Material& mat = *nextMat;
                        curMat = nextMat;

                        prog.setUniform("Kd", mat.Kd);
                        prog.setUniform("Ks", mat.Ks);
                        prog.setUniform("Ka", mat.Ka);
                        prog.setUniform("Ke", mat.Ke);
                        prog.setUniform("Tf", mat.Tf);
                        prog.setUniform("Ni", mat.Ni);

                        if( mat.map_Kd ) {
                            glActiveTexture(GL_TEXTURE0 + activeSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Kd->tex_id);
                            prog.setUniform("map_Kd", activeSlot++);
                        }
                        if( mat.map_Ks ) {
                            glActiveTexture(GL_TEXTURE0 + activeSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ks->tex_id);
                            prog.setUniform("map_Ks", activeSlot++);
                        }
                        if( mat.map_Ka ) {
                            glActiveTexture(GL_TEXTURE0 + activeSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ka->tex_id);
                            prog.setUniform("map_Ka", activeSlot++);
                        }
                        if( mat.map_Ns ) {
                            glActiveTexture(GL_TEXTURE0 + activeSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Ns->tex_id);
                            prog.setUniform("map_Ns", activeSlot++);
                        }
                        if( mat.map_Bump ) {
                            glActiveTexture(GL_TEXTURE0 + activeSlot);
		                    glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
                            prog.setUniform("map_Bump", activeSlot++);
                            if( !mat.bumpIsNormal ) {
                                prog.setUniform("bumpHeight", mat.bump_height);
                                prog.setUniform("texDelta", mat.tex_delta);
                            }
                        }
                    }

                    glBindVertexArray(ms.vert_array);
			        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ms.element_buf);
                    glDrawElements(GL_TRIANGLES, (GLsizei)(ms.tris.size()*3), GL_UNSIGNED_INT, 0);
                }
            }
        }

        fb.unbind();
    }
}
