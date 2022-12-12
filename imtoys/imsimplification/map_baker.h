//
//  2022-10-16 / im dong ye
// 
//	edit from : https://github.com/assimp/assimp/issues/203
//
//	target model의 normal을 저장하고 원본 모델의 world space normal을 계산한뒤
//	target normal으로 tbn을 계산해서 tangent space로 바꾼다.
// 
//	주의 bump_map이 아닌 texture은 원본과 공유한다.
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

		// target의 노멀을 저장해둘 framebuffer
		inline static Framebuffer* targetNormalMapPointer=nullptr;
		// target의 노멀을 사용해 baking할 framebuffer
		inline static Framebuffer* bakedNormalMapPointer=nullptr;
	private:
		inline static Program targetNormalProg = Program("Tangent Space Copier");
		inline static Program bakerProg = Program("Normal Map Baker");
	public:
		static void bakeNormalMap(std::string_view exportPath, Model *original, Model* target)
		{
			namespace fs = std::filesystem;
			if( bakedNormalMapPointer==nullptr ) {
				targetNormalMapPointer=new Framebuffer();
				targetNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
				targetNormalMapPointer->setSize(texSizes[selectedTexSizeIdx]);

				bakedNormalMapPointer=bakedNormalMapPointer=new Framebuffer();
				bakedNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
				bakedNormalMapPointer->setSize(texSizes[selectedTexSizeIdx]);
			}
			Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
			Framebuffer& targetNormalMap = *targetNormalMapPointer;

			if( glIsProgram(bakerProg.pid) ) {
				bakerProg.attatch("uv_view.vs").attatch("bake_normal.fs").link();
				targetNormalProg.attatch("uv_view.vs").attatch("map_wnor.fs").link();
			}

			/* 노멀맵을 따로 분리해준다. */ 
			std::map<std::shared_ptr<Texture>, std::shared_ptr<Texture>> oldAndNew;
			for( std::shared_ptr<Texture> tex : target->textures_loaded ) {
				if( tex->tag == "map_Bump" ) {
					auto newTex = tex->clone();
					oldAndNew[tex] = newTex;
					tex = newTex;
				}
			}
			int nrNewBump = 0;
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
					std::string newBumpPath = "new_bump"+std::to_string(nrNewBump)+".png";
					target->textures_loaded.push_back(std::make_shared<Texture>());
					target->textures_loaded.back()->path = newBumpPath;
					target->textures_loaded.back()->tag = "map_Bump";

					target->meshes[i]->textures.push_back(target->textures_loaded.back());
				}
			}

			/* 같은 노멀맵인 Mesh를 찾는다 */
			std::map<std::string, std::vector<Mesh *>> mergeByNormalMap;
			for( int i=0; i<original->meshes.size(); i++ ) {
				Mesh* oriMesh = original->meshes[i];
				bool hasBumpMap = false;
				for( std::shared_ptr<Texture> tex : oriMesh->textures ) {
					if( tex->tag == "map_Bump" ) {
						mergeByNormalMap[tex->path].push_back(oriMesh);
						hasBumpMap = true;
						break;
					}
				}
			}

			/* 원본의 모든 노멀맵에 대해 Baking */
			GLuint w = bakedNormalMap.width;
			GLuint h = bakedNormalMap.height;
			static GLubyte *data = new GLubyte[3 * w * h];
			for( auto &[filepath, meshes] : mergeByNormalMap ) {
				std::string fullPath(exportPath);
				fullPath += target->name;
				// 폴더없으면생성
				fs::path created_path(fullPath);
				if( !std::filesystem::is_directory(fullPath) )
					fs::create_directories(fullPath);
				fullPath += "/" + filepath;

				// target의 world normal 가져오기
				GLuint pid = targetNormalProg.use();
				targetNormalMap.bind();
				for( Mesh *mesh : target->meshes ) {
					mesh->draw(0); // not bind textures
				}
				targetNormalMap.unbind();

				// 맵 베이킹 시작
				pid = bakerProg.use();
				bakedNormalMap.bind();

				// target normal 마지막 슬롯으로 넘기기
				glActiveTexture(GL_TEXTURE31);
				glBindTexture(GL_TEXTURE_2D, targetNormalMap.color_tex);
				setUniform(pid, "map_TargetNormal", 31);// to sampler2d
				glActiveTexture(GL_TEXTURE0);

				// 원본의 노멀과 bumpmap, Target의 노멀으로 그린다.
				for( Mesh *mesh : original->meshes ) {
					mesh->draw(pid);
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
			}

			/* 새로운 노멀맵으로 로딩 */
			std::string modelPath(exportPath);
			modelPath += target->name;

			for( std::shared_ptr<Texture> tex : target->textures_loaded ) {
				if( tex->tag == "map_Bump" ) {
					tex->reload(modelPath + "/" + tex->path, GL_RGB8);
				}
			}

			Logger::get() << "reload normal map" << Logger::endll;
		}

	};
}

#endif