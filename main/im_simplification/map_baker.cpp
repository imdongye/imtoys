#include "map_baker.h"
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

namespace
{
	 // target의 노멀을 저장해둘 framebuffer
	lim::Framebuffer* targetNormalMapPointer=nullptr;
	 // target의 노멀을 사용해 baking할 framebuffer
	lim::Framebuffer* bakedNormalMapPointer=nullptr;
	lim::Program* normalDrawProg=nullptr;
	lim::Program* bakerProg=nullptr;

    void resourceInit(int texSize = 256)
    {
        if( bakedNormalMapPointer==nullptr ) {
            targetNormalMapPointer= new lim::Framebuffer();
            targetNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
            targetNormalMapPointer->resize(texSize);

            bakedNormalMapPointer= new lim::Framebuffer();
            bakedNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
            bakedNormalMapPointer->resize(texSize);
        }
        if( bakerProg==nullptr ) {
            normalDrawProg = new lim::Program("normal to tex coord", "im_simplification/");
            normalDrawProg->attatch("uv_view.vs").attatch("wnor_out.fs").link();

            bakerProg = new lim::Program("normal map baker", "im_simplification/");
            bakerProg->attatch("uv_view.vs").attatch("bake_normal.fs").link();
        }
    }
    void resourceDeinit()
    {
        delete targetNormalMapPointer; targetNormalMapPointer=nullptr;
        delete bakedNormalMapPointer;  bakedNormalMapPointer=nullptr;
        delete normalDrawProg;         normalDrawProg=nullptr;
        delete bakerProg;              bakerProg=nullptr;
    }
}

// 출력할때 scene에 텍스쳐 경로가 어떻게 되는거지

namespace lim
{
    void bakeNormalMap(std::string_view exportPath, Model* to, Model* from, int texSize)
    {
        namespace fs = std::filesystem;
        resourceInit(texSize);
        Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
        Framebuffer& targetNormalMap = *targetNormalMapPointer;

        // 노멀 맵을 사용하는 메쉬들을 찾아서 자료구조에 저장.
        unordered_map< int, vector<pair<Mesh*, Mesh*>> > mergeByNormalMap;
        for( int i=0; i<from->meshes.size(); i++ ) {
            Mesh* toMs = to->meshes[i];
            Mesh* fmMs = from->meshes[i];
            if( toMs->material==nullptr || toMs->material->map_Bump==nullptr ) 
                continue;
            int bumpMatIdx = findIdx(to->materials, toMs->material);
            mergeByNormalMap[bumpMatIdx].push_back( make_pair(toMs,fmMs) );
        }


        GLubyte* fileData = new GLubyte[3 * texSize * texSize];

        for( auto& [bumpMatIdx, meshes] : mergeByNormalMap ) {
            Material* toMat = to->materials[bumpMatIdx];
            Material* fmMat = from->materials[bumpMatIdx];

            // from의 model space normal 가져오기
            targetNormalMap.bind();
            normalDrawProg->use();
            for( auto& [toMesh, fromMesh] : meshes ) {
                fromMesh->drawGL();
            }
            targetNormalMap.unbind();

            /* 맵 베이킹 시작 */
            bakedNormalMap.bind();
            bakerProg->use();

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, targetNormalMap.color_tex);
            bakerProg->setUniform("map_TargetNormal", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, fmMat->map_Bump->tex_id);
            bakerProg->setUniform("map_Bump", 1);

            bakerProg->setUniform("mapFlags", fmMat->map_Flags);
            bakerProg->setUniform("texDelta", fmMat->texDelta);
            bakerProg->setUniform("bumpHeight", fmMat->bumpHeight);

            // 원본의 노멀과 bumpmap, Target의 노멀으로 그린다.
            for( auto& [toMesh, fromMesh] : meshes ) {
                toMesh->drawGL();
            }
            bakedNormalMap.unbind();


            /* png로 저장 */
            glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, texSize, texSize, GL_RGB, GL_UNSIGNED_BYTE, fileData);

            // 덮어쓴다.
            stbi_flip_vertically_on_write(true);

            const size_t lastSlashPos = from->path.find_last_of("/\\");
            std::string internalModelPath = fmMat->map_Bump->path.c_str() + lastSlashPos;
            std::string modelDir = (lastSlashPos == 0) ? "" : from->path.substr(0, lastSlashPos) + "/"; // todo: check

            std::string texPath = modelDir + internalModelPath;
            toMat->map_Bump->path = texPath;
            stbi_write_png(texPath.c_str(), texSize, texSize, 3, fileData, texSize * 3);
            log::pure("baked in %s\n\n", texPath.c_str());
        }


        /* 새로운 노멀맵으로 로딩 */
        for( Texture* tex : to->textures_loaded ) {
            if( tex->tag == "map_Bump" ) {
                tex->initFromImage(tex->path, GL_RGB8);
            }
        }

        log::pure("reload normal map\n");

        resourceDeinit();
    }
}