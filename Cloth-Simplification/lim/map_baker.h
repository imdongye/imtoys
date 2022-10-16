//
//  2022-09-27 / im dong ye
// 
//	edit from : https://github.com/assimp/assimp/issues/203
//

#ifndef MAP_BAKER_H
#define MAP_BAKER_H

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <filesystem>
#include <vector>
#include <glm/glm.hpp>
#include "model.h"
#include "program.h"
#include "framebuffer.h"

namespace lim
{
	class MapBaker
	{
	public:
		inline static int selectedTexSizeIdx = 2;
		inline static const int nrTexSize=6;
		inline static int texSizes[]={256, 512, 1024, 2048, 4096, 8192};
		inline static const char* texSizeStrs[]={"256", "512", "1024", "2048", "4096", "8192"};

		inline static Framebuffer* bakedNormalMapPointer=nullptr;
	private:
		inline static Program bakerProg = Program("Normal Map Baker");
	public:
		static void bakeNormalMap(std::string_view exportPath, Model *original, Model* target=nullptr)
		{
			namespace fs = std::filesystem;
			if( bakerProg.ID==0 ) {
				bakerProg.attatch("uv_view.vs").attatch("uv_view.fs").link();
			}
			if( bakedNormalMapPointer==nullptr ) {
				bakedNormalMapPointer=new Framebuffer();
				bakedNormalMapPointer->clearColor = glm::vec4(0.5, 0.5, 1, 1);
			}
			Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
			if( bakedNormalMap.width!= texSizes[selectedTexSizeIdx] ) {
				bakedNormalMap.clearColor = glm::vec4(0.5, 0.5, 1, 1);
				bakedNormalMap.resize(texSizes[selectedTexSizeIdx]);
			}

			std::map<std::string, std::vector<Mesh *>> mergeByNormalMap;
			GLuint pid = bakerProg.use();
			GLuint w = bakedNormalMap.width;
			GLuint h = bakedNormalMap.height;
			static GLubyte *data = new GLubyte[3 * w * h];

			for( Mesh *mesh : original->meshes ) {
				for( GLuint texIdx : mesh->texIdxs ) {
					Texture &tex = original->textures_loaded[texIdx];
					if( tex.type == "map_Bump" ) {
						mergeByNormalMap[tex.path].push_back(mesh);
					}
				}
			}
			for( auto &[filepath, meshes] : mergeByNormalMap ) {
				std::string fullPath(exportPath);
				// simp된 targetviewport가 있으면 그쪽 폴더로 생성함.
				fullPath += (target != nullptr) ? target->name : original->name;
				// 폴더없으면생성
				fs::path created_path(fullPath);
				if( !std::filesystem::is_directory(fullPath) )
					fs::create_directories(fullPath);
				fullPath += "/" + filepath;

				bakedNormalMap.bind();
				for( Mesh *mesh : original->meshes ) {
					mesh->draw(pid, original->textures_loaded);
				}
				bakedNormalMap.unbind();

				glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// 덮어쓴다.
				stbi_flip_vertically_on_write(true);
				stbi_write_png(fullPath.c_str(), w, h, 3, data, w * 3);

				Logger::get() << "baked in " << fullPath.c_str() << Logger::endll;
				if( target != nullptr ) {
					target->reloadNormalMap(fullPath);
					Logger::get() << "reload normal map" << Logger::endll;
				}
			}

		}
	};

}

#endif