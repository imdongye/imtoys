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
        namespace fs = std::filesystem;
        Framebuffer targetNormalMap;
        targetNormalMap.clear_color = glm::vec4(0.5, 0.5, 1, 1);
        targetNormalMap.resize(texSize);

        Framebuffer bakedNormalMap;
        bakedNormalMap.clear_color = glm::vec4(0.5, 0.5, 1, 1);
        bakedNormalMap.resize(texSize);

        Program normalDrawProg("normal to tex coord", "im_simplification/");
        normalDrawProg.attatch("uv_view.vs").attatch("wnor_out.fs").link();

        Program bakerProg("normal map baker", "im_simplification/");
        bakerProg.attatch("uv_view.vs").attatch("bake_normal.fs").link();



        // 노멀 맵을 사용하는 메쉬들을 찾아서 자료구조에 저장.
        unordered_map< int, vector<pair<Mesh*, Mesh*>> > mergeByNormalMap;
        for( int i=0; i<src.meshes.size(); i++ ) {
            Mesh* srcMs = src.meshes[i];
            Mesh* dstMs = dst.meshes[i];
            if( dstMs->material==nullptr || dstMs->material->map_Bump==nullptr ) 
                continue;
            int bumpMatIdx = findIdx(dst.materials, dstMs->material);
            mergeByNormalMap[bumpMatIdx].push_back( make_pair(dstMs,srcMs) );
        }


        GLubyte* fileData = new GLubyte[3 * texSize * texSize];

        for( auto& [bumpMatIdx, meshes] : mergeByNormalMap ) {
            const Material& srcMat = *src.materials[bumpMatIdx];
            Material& dstMat = *dst.materials[bumpMatIdx];

            /* src의 model space normal 텍스쳐로 가져오기 */
            targetNormalMap.bind();
            normalDrawProg.use();
            for( auto& [toMesh, fromMesh] : meshes ) {
                fromMesh->drawGL();
            }
            targetNormalMap.unbind();


            /* 맵 베이킹 시작 */
            bakedNormalMap.bind();
            bakerProg.use();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, targetNormalMap.color_tex);
            bakerProg.setUniform("map_TargetNormal", 0);

            bakerProg.setUniform("map_Flags", srcMat.map_Flags);
            if ( srcMat.map_Flags & (Material::MF_Bump|Material::MF_Nor) ) {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, srcMat.map_Bump->tex_id);
                bakerProg.setUniform("map_Bump", 1);
                bakerProg.setUniform("texDelta", srcMat.texDelta);
                bakerProg.setUniform("bumpHeight", srcMat.bumpHeight);
            }
            // 원본의 노멀과 bumpmap, Target의 노멀으로 그린다.
            for( auto& [toMesh, fromMesh] : meshes ) {
                toMesh->drawGL();
            }
            bakedNormalMap.unbind();


            /* png로 저장 */
            glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, texSize, texSize, GL_RGB, GL_UNSIGNED_BYTE, fileData);

            // 새로운 노멀맵으로 텍스쳐 업데이트
            dstMat.map_Bump->initGL(fileData); // todo : test and fbo to tex로 최적화

            std::string internalModelPath = srcMat.map_Bump->path.c_str() + src.path.size();
            std::string texPath = dst.path + internalModelPath;
            const size_t lastSlashPos = texPath.find_last_of("/\\");
            
            // 덮어쓴다. 
            fs::path createdPath(texPath.substr(0, lastSlashPos));
            if (!std::filesystem::is_directory(createdPath))
                fs::create_directories(createdPath);
            stbi_flip_vertically_on_write(true);
            stbi_write_png(texPath.c_str(), texSize, texSize, 3, fileData, texSize * 3);

            dstMat.map_Bump->path = texPath;
            dstMat.map_Flags &= ~Material::MF_Bump;
            dstMat.map_Flags |=  Material::MF_Nor;
            
            log::pure("baked in %s\n\n", texPath.c_str());
        }

    }
}