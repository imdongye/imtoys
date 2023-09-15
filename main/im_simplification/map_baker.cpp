#include "map_baker.h"
#include <limbrary/framebuffer.h>
#include <limbrary/program.h>
#include <filesystem>
#include <stb_image_write.h>

namespace
{
    int selectedTexSizeIdx = 2;
	const int nrTexSize=6;
	int texSizes[]={256, 512, 1024, 2048, 4096, 8192};
	const char* texSizeStrs[]={"256", "512", "1024", "2048", "4096", "8192"};

	 // target의 노멀을 저장해둘 framebuffer
	lim::Framebuffer* targetNormalMapPointer=nullptr;
	 // target의 노멀을 사용해 baking할 framebuffer
	lim::Framebuffer* bakedNormalMapPointer=nullptr;
	lim::Program* normalDrawProg=nullptr;
	lim::Program* bakerProg=nullptr;

    void resourceInit()
    {
        if( bakedNormalMapPointer==nullptr ) {
            targetNormalMapPointer= new lim::Framebuffer();
            targetNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
            targetNormalMapPointer->resize(texSizes[selectedTexSizeIdx]);

            bakedNormalMapPointer= new lim::Framebuffer();
            bakedNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
            bakedNormalMapPointer->resize(texSizes[selectedTexSizeIdx]);
        }
        if( bakerProg==nullptr ) {
            normalDrawProg = new lim::Program("normal to tex coord", "imsimplification/");
            normalDrawProg->attatch("uv_view.vs").attatch("map_wnor.fs").link();

            bakerProg = new lim::Program("normal map baker", "imsimplification/");
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

namespace lim
{
    void bakeNormalMap(std::string_view exportPath, Model* original, Model* target)
    {
        namespace fs = std::filesystem;
        resourceInit();
        Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
        Framebuffer& targetNormalMap = *targetNormalMapPointer;

        
        /* mesh에도 새로 만든 노멀맵을 연결해준다. */
        for( int i=0; i<target->meshes.size(); i++ ) {
            bool hasBumpMap = false;
            Mesh* targetMesh = target->meshes[i];
            for( std::shared_ptr<Texture> tex : targetMesh->textures ) {
                if( tex->tag == "map_Bump" ) {
                    tex = oldAndNew[tex];
                    hasBumpMap = true;
                    break;
                }
            }
            /* 노멀맵이 없었다면 Target에 메쉬마다 다르게 생성 */
            // Todo : 같이 그릴수있는 노멀맵을 하나로 합쳐야한다. Tex coord를 공유하는 mesh들
            if( !hasBumpMap ) {
                /*
                std::string newBumpPath = "new_bump"+std::to_string(nrNewBump)+".png";
                target->textures_loaded.push_back(std::make_shared<Texture>());
                target->textures_loaded.back()->path = newBumpPath;
                target->textures_loaded.back()->tag = "map_Bump";
                target->meshes[i]->textures.push_back(target->textures_loaded.back());
                */
                log::err("No bump map in mesh: %s\n", targetMesh->name.c_str());

            }
        }

        /* 같은 노멀맵인 Mesh를 찾는다 */
        std::map<const char*, std::vector< std::pair<Mesh*, Mesh*> >> mergeByNormalMap;
        for( int i=0; i<original->meshes.size(); i++ ) {
            Mesh* oriMesh = original->meshes[i];
            Mesh* targetMesh = target->meshes[i];
            bool hasBumpMap = false;
            for( std::shared_ptr<Texture> tex : oriMesh->textures ) {
                if( tex->tag == "map_Bump" ) {
                    mergeByNormalMap[tex->internal_model_path].push_back( std::make_pair(oriMesh, targetMesh) );
                    hasBumpMap = true;
                    break;
                }
            }
        }

        /* Simplified 모델 경로 생성 */
        std::string modelDir(exportPath);
        modelDir += target->name + "/";
        fs::path created_path(modelDir);
        if( !std::filesystem::is_directory(modelDir) )
            fs::create_directories(modelDir);

        /* 원본의 모든 노멀맵에 대해 Baking */
        GLuint w = bakedNormalMap.width;
        GLuint h = bakedNormalMap.height;
        static GLubyte *data = new GLubyte[3 * w * h];
        for( auto& [internalModelPath, meshes] : mergeByNormalMap ) {
            // target의 world normal 가져오기
            GLuint pid = normalDrawProg->use();
            targetNormalMap.bind();
            for( auto& [oriMesh, targetMesh] : meshes ) {
                glBindVertexArray(targetMesh->VAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, targetMesh->EBO);
                glDrawElements(targetMesh->draw_mode, static_cast<GLuint>(targetMesh->indices.size()), GL_UNSIGNED_INT, 0);
            }
            targetNormalMap.unbind();

            /* 맵 베이킹 시작 */
            pid = bakerProg->use();
            bakedNormalMap.bind();

            // target normal 마지막 슬롯으로 넘기기
            glActiveTexture(GL_TEXTURE31);
            glBindTexture(GL_TEXTURE_2D, targetNormalMap.color_tex);
            setUniform(pid, "map_TargetNormal", 31);// to sampler2d

            // 원본의 노멀과 bumpmap, Target의 노멀으로 그린다.
            for( auto& [oriMesh, targetMesh] : meshes ) {
                glBindVertexArray(oriMesh->VAO);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oriMesh->EBO);
                glDrawElements(oriMesh->draw_mode, static_cast<GLuint>(oriMesh->indices.size()), GL_UNSIGNED_INT, 0);
            }
            bakedNormalMap.unbind();

            /* png로 저장 */
            glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);

            // 덮어쓴다.
            stbi_flip_vertically_on_write(true);
            std::string texPath = modelDir+std::string(internalModelPath);
            stbi_write_png(texPath.c_str(), w, h, 3, data, w * 3);
            log::pure("baked in %s\n\n", texPath.c_str());
        }

        /* 새로운 노멀맵으로 로딩 */
        for( std::shared_ptr<Texture> tex : target->textures_loaded ) {
            if( tex->tag == "map_Bump" ) {
                tex->reload(modelDir + tex->internal_model_path, GL_RGB8);
            }
        }

        log::pure("reload normal map\n");

        resourceDeinit();
    }
}