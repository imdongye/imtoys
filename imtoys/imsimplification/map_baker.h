//
//  2022-10-16 / im dong ye
// 
//	edit from : https://github.com/assimp/assimp/issues/203
//
//	target model의 normal을 저장하고 원본 모델의 world space normal을 계산한뒤
//	target normal으로 tbn을 계산해서 tangent space로 바꾼다.
//


#ifndef MAP_BAKER_H
#define MAP_BAKER_H

namespace lim
{
	class MapBaker
	{
	public:
		inline static int selectedTexSizeIdx = 2;
		inline static const int nrTexSize=6;
		inline static int texSizes[]={256, 512, 1024, 2048, 4096, 8192};
		inline static const char* texSizeStrs[]={"256", "512", "1024", "2048", "4096", "8192"};

		inline static Framebuffer* targetNormalMapPointer=nullptr;
		inline static Framebuffer* bakedNormalMapPointer=nullptr;
	private:
		inline static Program bakerProg = Program("Normal Map Baker");
		inline static Program targetNormalProg = Program("Tangent Space Copier");
	public:
		static void bakeNormalMap(std::string_view exportPath, Model *original, Model* target=nullptr)
		{
			namespace fs = std::filesystem;
			if( glIsProgram( bakerProg.pid ) ) {
				bakerProg.attatch("uv_view.vs").attatch("bake_normal.fs").link();
				targetNormalProg.attatch("uv_view.vs").attatch("map_wnor.fs").link();
			}
			if( bakedNormalMapPointer==nullptr ) { // Todo
				bakedNormalMapPointer=bakedNormalMapPointer=new Framebuffer();
				bakedNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);

				targetNormalMapPointer=new Framebuffer();
				targetNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
			}
			Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
			Framebuffer& targetNormalMap = *targetNormalMapPointer;
			if( bakedNormalMap.width!= texSizes[selectedTexSizeIdx] ) {
				bakedNormalMap.setSize(texSizes[selectedTexSizeIdx]);

				targetNormalMap.setSize(texSizes[selectedTexSizeIdx]);
			}

			std::map<std::string, std::vector<Mesh *>> mergeByNormalMap;
			for( int i=0; i<original->meshes.size(); i++ ) {
				Mesh* oriMesh = original->meshes[i];
				bool hasBumpMap = false;
				for( GLuint texIdx : oriMesh->texIdxs ) {
					Texture &tex = *original->textures_loaded[texIdx];
					if( tex.tag == "map_Bump" ) {
						mergeByNormalMap[tex.path].push_back(oriMesh);
						hasBumpMap = true;
						break;
					}
				}
				if( hasBumpMap==false ) {
					if( target!=nullptr ) {
						target->meshes[i]->texIdxs.push_back(target->textures_loaded.size());
					}
					mergeByNormalMap["new_bump.png"].push_back(oriMesh);
				}
			}

			GLuint w = bakedNormalMap.width;
			GLuint h = bakedNormalMap.height;
			static GLubyte *data = new GLubyte[3 * w * h];
			for( auto &[filepath, meshes] : mergeByNormalMap ) {
				std::string fullPath(exportPath);
				// simp된 targetviewport가 있으면 그쪽 폴더로 생성함.
				fullPath += (target != nullptr) ? target->name : original->name;
				// 폴더없으면생성
				fs::path created_path(fullPath);
				if( !std::filesystem::is_directory(fullPath) )
					fs::create_directories(fullPath);
				fullPath += "/" + filepath;

				/* target의 normal 가져오기 */
				if( target!=nullptr ) {
					GLuint pid = targetNormalProg.use();
					targetNormalMap.bind();
					for( Mesh *mesh : target->meshes ) {
						mesh->draw(0); // not bind textures
					}
					targetNormalMap.unbind();
				}

				/* 맵 베이킹 */
				GLuint pid = bakerProg.use();
				bakedNormalMap.bind();
				// target normal 마지막 슬롯으로 넘기기
				if( target!=nullptr ) {
					glActiveTexture(GL_TEXTURE31);
					glBindTexture(GL_TEXTURE_2D, targetNormalMap.color_tex);
					setUniform(pid, "map_TargetNormal", 31);// to sampler2d
					glActiveTexture(GL_TEXTURE0);
				}
				for( Mesh *mesh : original->meshes ) {
					mesh->draw(pid, original->textures_loaded);
				}
				bakedNormalMap.unbind();

				/* png로 저장 */
				glBindFramebuffer(GL_FRAMEBUFFER, bakedNormalMap.fbo);
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				// 덮어쓴다.
				stbi_flip_vertically_on_write(true);
				stbi_write_png(fullPath.c_str(), w, h, 3, data, w * 3);
				Logger::get() << "baked in " << fullPath.c_str() << Logger::endll;

				/* 새로운 노멀맵으로 로딩 */
				if( target != nullptr ) {
					bool hasBumpMap = false;
					for( Texture* tex : target->textures_loaded ) {
						if( tex->tag == "map_Bump" ) {
							tex->reload(fullPath, GL_RGB8);
							hasBumpMap = true;
							break;
						}
					}
					if( hasBumpMap==false ) {
						Texture* newTex = new Texture(fullPath, GL_RGB8);\
						newTex->path = fullPath.substr(fullPath.find_last_of('/')+1);
						newTex->tag = "map_Bump";
						target->textures_loaded.push_back(newTex);
						// Todo:
					}
					Logger::get() << "reload normal map" << Logger::endll;
				}
			}

		}
	};

}

#endif