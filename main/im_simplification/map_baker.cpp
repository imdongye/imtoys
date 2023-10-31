#include "simplify.h"
#include <limbrary/framebuffer.h>
#include <limbrary/program.h>
#include <filesystem>
#include <stb_image_write.h>
#include <limbrary/model_view/mesh.h>
#include <limbrary/utils.h>
#include <unordered_map>
#include <vector>
#include <stack>
using namespace std;

// 출력할때 scene에 텍스쳐 경로가 어떻게 되는거지

namespace lim
{
    void bakeNormalMap(const Model& src, Model& dst, int texSize)
    {
        if(src.my_meshes.size() != dst.my_meshes.size()) {
			log::err("not mached model in bake normal map\n\n");
            return;
        }
        Framebuffer srcNormalMap;
        srcNormalMap.clear_color = {0.5, 0.5, 1, 1};
        srcNormalMap.resize(texSize);

        Program normalDrawProg;
        normalDrawProg.name = "normal to tex coord";
        normalDrawProg.home_dir = "im_simplification";
        normalDrawProg.attatch("uv_view.vs").attatch("baker_draw_ori_nor.fs").link();

        Program bakerProg;
        bakerProg.name = "normal map baker";
        bakerProg.home_dir = "im_simplification";
        bakerProg.attatch("uv_view.vs").attatch("baker_ori_nor_to_simp_ts.fs").link();


        // 노멀 맵을 사용하는 메쉬들을 찾아서 자료구조에 저장.  Todo: 사용안해도 저장
        unordered_map< int, vector<pair<const Mesh*, const Mesh*>> > mergeByNormalMap;
        src.root.treversal([&](const Mesh* srcMs, const Material* srcMat) {
            if( srcMat==nullptr || srcMat->map_Bump==nullptr ) 
                return;
            if( srcMs->uvs.size()==0 ) {
                log::err("no uvs in mesh when bakeNorMap\n\n");
                return;
            }
            int bumpMatIdx = findIdx(src.my_materials, (Material*)srcMat);
            const Mesh* dstMs = dst.my_meshes[ findIdx(src.my_meshes, (Mesh*)srcMs) ];

            mergeByNormalMap[bumpMatIdx].push_back( make_pair(srcMs, dstMs) );
        });



        for( auto& [bumpMatIdx, meshes] : mergeByNormalMap ) {
            const Material& srcMat = *src.my_materials[bumpMatIdx];
            Material& dstMat = *dst.my_materials[bumpMatIdx];

            /* src의 world space normal bump맵 적용해서 텍스쳐로 가져오기 */
            srcNormalMap.bind();
            normalDrawProg.use();
            normalDrawProg.setUniform("map_Flags", srcMat.map_Flags);
            if ( srcMat.map_Flags & (Material::MF_HEIGHT|Material::MF_NOR) ) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, srcMat.map_Bump->tex_id);
                normalDrawProg.setUniform("map_Bump", 0);
                normalDrawProg.setUniform("texDelta", srcMat.texDelta);
                normalDrawProg.setUniform("bumpHeight", srcMat.bumpHeight);
            }
            // 원본의 노멀과 bumpmap, Target의 노멀으로 그린다.
            for( auto& [srcMs, dstMs] : meshes ) {
                srcMs->drawGL();
            }
            srcNormalMap.unbind();

            /* 맵 베이킹 시작 */
            dstMat.map_Bump->width = texSize;
            dstMat.map_Bump->height = texSize;
            dstMat.map_Bump->internal_format = GL_RGB8;
            dstMat.map_Bump->initGL();
            GLuint srcFbo = 0;
            glGenFramebuffers(1, &srcFbo);
            glBindFramebuffer(GL_FRAMEBUFFER, srcFbo);
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstMat.map_Bump->tex_id, 0 );
            glViewport(0, 0, texSize, texSize);
            glClearColor(0.5f, 0.5f, 1.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            bakerProg.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, srcNormalMap.color_tex);
            bakerProg.setUniform("map_OriNormal", 0);
            
            for( auto& [srcMs, dstMs] : meshes ) {
                dstMs->drawGL();
            }

            glBindTexture(GL_TEXTURE_2D, dstMat.map_Bump->tex_id);
            glGenerateMipmap(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &srcFbo);

            dstMat.map_Flags &= ~Material::MF_HEIGHT;
            dstMat.map_Flags |=  Material::MF_NOR;
            
            log::pure("bake done  %s\n\n", dstMat.map_Bump->name.c_str());
        }
    }

    void convertBumpMapToNormalMap(Model& md)
    {
        Program bumpToNorProg;
        bumpToNorProg.name = "bump to nor";
        bumpToNorProg.home_dir = "im_simplification";
        bumpToNorProg.attatch("uv_view.vs").attatch("baker_bump_to_nor.fs").link();

        unordered_map< int,  vector<const Mesh*> > mergeByNormalMap;

        md.root.treversal([&](const Mesh* ms, const Material* mat) {
            if( mat==nullptr || (mat->map_Flags|Material::MF_HEIGHT)==0 ) 
                return;
            if( ms->uvs.size()==0 ) {
                log::err("no uvs in mesh when bakeNorMap\n\n");
                return;
            }
            int bumpMatIdx = findIdx(md.my_materials, (Material*)mat);
            const Mesh* dstMs = md.my_meshes[ findIdx(md.my_meshes, (Mesh*)ms) ];

            mergeByNormalMap[bumpMatIdx].push_back( ms );
        });


        if( mergeByNormalMap.size()==0 ) {
            log::err("there are no bumpmap in %s\n\n", md.name.c_str());
            return;
        }

        GLuint norFbo;
        glGenFramebuffers( 1, &norFbo );
        glBindFramebuffer( GL_FRAMEBUFFER, norFbo );
		glDisable(GL_DEPTH_TEST);


        for( auto& [bumpMatIdx, meshes] : mergeByNormalMap ) {
            Material& mat = *md.my_materials[bumpMatIdx];
            GLuint norTex;
            GLsizei norWidth = mat.map_Bump->width;
            GLsizei norHeight = mat.map_Bump->height;
            glGenTextures(1, &norTex);
            glBindTexture(GL_TEXTURE_2D, norTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, norWidth, norHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
            glBindFramebuffer(GL_FRAMEBUFFER, norFbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, norTex, 0);
            
            /* draw to fb*/
		    glViewport(0, 0, norWidth, norHeight);
            glClearColor(0.5f, 0.5f, 1.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            bumpToNorProg.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            bumpToNorProg.setUniform("map_Bump", 0);
            bumpToNorProg.setUniform("map_Flags", mat.map_Flags);
            bumpToNorProg.setUniform("texDelta", mat.texDelta);
            bumpToNorProg.setUniform("bumpHeight", mat.bumpHeight);
            for( const Mesh* ms : meshes ) {
                ms->drawGL();
            }

            glBindTexture(GL_TEXTURE_2D, norTex);
            glGenerateMipmap(GL_TEXTURE_2D);

            // 텍스쳐 복사 방법 3가지

            // 1. [Tex->Fbo] framebuffer에 그리기
            // mat.map_Bump->initGL();
            // glBindFramebuffer(GL_FRAMEBUFFER, norFbo);
            // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mat.map_Bump->tex_id, 0);
            // glViewport(0, 0, norWidth, norHeight);
            // glClearColor(0.5f, 0.5f, 1.f, 1.f);
            // glClear(GL_COLOR_BUFFER_BIT);
            // drawTexToQuad(norTex, 1.f);
            // glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            // glGenerateMipmap(GL_TEXTURE_2D);

            // 2. [Fbo->Tex] framebuffer에서 복사하기 (크기 고정)
            glBindFramebuffer(GL_READ_FRAMEBUFFER, norFbo);
            glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, norWidth, norHeight);
            //glCopyTexImage2D() // 복사하면서 생성
            glGenerateMipmap(GL_TEXTURE_2D);
		    glBindTexture(GL_TEXTURE_2D, 0);

            // 3. [Fbo->Fbo] framebuffer에서 framebuffer로 복사하기
            // GLuint oriFbo;
            // glGenFramebuffers(1, &oriFbo);
            // glBindFramebuffer(GL_FRAMEBUFFER, oriFbo);
            // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mat.map_Bump->tex_id, 0);
            // glDrawBuffer(GL_COLOR_ATTACHMENT0);
            // glBindFramebuffer(GL_READ_FRAMEBUFFER, norFbo);
            // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oriFbo);
            // glBlitFramebuffer(0, 0,  norWidth,  norHeight, 0, 0,  norWidth, norHeight,
            //                   GL_COLOR_BUFFER_BIT, GL_NEAREST);
            // glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            // glBindTexture(GL_TEXTURE_2D, mat.map_Bump->tex_id);
            // glGenerateMipmap(GL_TEXTURE_2D);
            // glDeleteFramebuffers(1, &oriFbo);


            mat.map_Flags &= ~Material::MF_HEIGHT;
            mat.map_Flags |=  Material::MF_NOR;
            
            glDeleteTextures(1, &norTex);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers( 1, &norFbo );
        glEnable(GL_DEPTH_TEST);
        log::pure("done convert bump map to normal map\n");
    }
}