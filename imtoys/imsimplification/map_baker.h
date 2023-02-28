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
//	Todo:
//	노멀맵이 없는경우 diffuse맵등으로 구분하여 새로운 노멀맵에 그려주고
//	assimp scene의 texture을 조작해서 연결한뒤 export해야함.


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
		inline static Program* normalDrawProg=nullptr;
		inline static Program* bakerProg=nullptr;
	public:
		static void bakeNormalMap(std::string_view exportPath, Model *original, Model* target)
		{
			namespace fs = std::filesystem;
			MapBaker::enable();
			Framebuffer& bakedNormalMap = *bakedNormalMapPointer;
			Framebuffer& targetNormalMap = *targetNormalMapPointer;

			/* 노멀맵이 공유되지 않게 새로 clone한다. */ 
			std::map<std::shared_ptr<Texture>, std::shared_ptr<Texture>> oldAndNew;
			for( std::shared_ptr<Texture> tex : target->textures_loaded ) {
				if( tex->tag == "map_Bump" ) {
					auto newTex = tex->clone();
					oldAndNew[tex] = newTex;
					tex = newTex;
				}
			}
			
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
					Logger::get(1).log("No bump map in mesh: %s\n", targetMesh->name.c_str());

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
					targetMesh->draw(0); // not bind textures
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
					oriMesh->draw(pid);
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
				Logger::get() << "baked in " << texPath.c_str() << Logger::endll;
			}

			/* 새로운 노멀맵으로 로딩 */
			for( std::shared_ptr<Texture> tex : target->textures_loaded ) {
				if( tex->tag == "map_Bump" ) {
					tex->reload(modelDir + tex->internal_model_path, GL_RGB8);
				}
			}

			Logger::get() << "reload normal map" << Logger::endll;
		}
		static void enable()
		{
			if( bakedNormalMapPointer==nullptr ) {
				targetNormalMapPointer=new Framebuffer();
				targetNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
				targetNormalMapPointer->resize(texSizes[selectedTexSizeIdx]);

				bakedNormalMapPointer=new Framebuffer();
				bakedNormalMapPointer->clear_color = glm::vec4(0.5, 0.5, 1, 1);
				bakedNormalMapPointer->resize(texSizes[selectedTexSizeIdx]);
			}
			if( bakerProg==nullptr ) {
				normalDrawProg = new Program("normal to tex coord", "imsimplification/");
				normalDrawProg->attatch("uv_view.vs").attatch("map_wnor.fs").link();

				bakerProg = new Program("normal map baker", "imsimplification/");
				bakerProg->attatch("uv_view.vs").attatch("bake_normal.fs").link();
			}
		}
		static void disable()
		{
			delete targetNormalMapPointer; targetNormalMapPointer=nullptr;
			delete bakedNormalMapPointer;  bakedNormalMapPointer=nullptr;
			delete normalDrawProg;         normalDrawProg=nullptr;
			delete bakerProg;              bakerProg=nullptr;
		}
	};
}

#endif