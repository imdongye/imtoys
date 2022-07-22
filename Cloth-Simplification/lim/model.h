//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
//  TODO list:
//  1. rigging
//  2. cleanUp
//

#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>
#include <vector>

#include "program.h"

#define MAX_BONE_INFLUENCE 4

// 4byte * (3+3+2+3+3+4+3) = 21
// offsetof(Vertex, Normal) = 12
struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 uv;
    glm::vec3 tangent;
    glm::vec3 bitangent;
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    GLuint id;
    std::string type;
    std::string path; // relative path+filename or only filename
}; 

struct Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    GLuint VAO = 0;
private:
    GLuint VBO, EBO;

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        // VAO setting //
        // - position
        glEnableVertexAttribArray(0);	
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        // - normal
        glEnableVertexAttribArray(1);	
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));
        // - tex coord
        glEnableVertexAttribArray(2);	
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        // - tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        // - bi tangnet
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

		// - bone ids
		glEnableVertexAttribArray(5);
		glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
		// - weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        glBindVertexArray(0);
    }
public:
    Mesh(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, std::vector<Texture>& textures)
        : VAO(0)
    {
        this->vertices = vertices;
        this->indices = indices;
        this->textures = textures;

        // now that we have all the required data, set the vertex buffers and its attribute pointers.
        setupMesh();
    }
    ~Mesh() { 
        glDeleteVertexArrays(1, &VAO);
    }
    void draw(Program& program) {
        // texture unifrom var name texture_specularN ...
        GLuint diffuseNr  = 0;
        GLuint specularNr = 0;
        GLuint normalNr   = 0;
        GLuint ambientNr  = 0;
        GLuint loc = 0;

        for(GLuint i = 0; i < textures.size(); i++) {
            std::string type = textures[i].type;
            // uniform samper2d nr is start with 1
            // omit 0th
            int backNum = 0;
            if( type=="map_Kd" )        backNum = ++diffuseNr;
            else if( type=="map_Ks" )   backNum = ++specularNr;
            else if( type=="map_Bump" ) backNum = ++normalNr;
            else if( type=="map_Ka" )   backNum = ++ambientNr;
                
            std::string varName = type + std::to_string(backNum);
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
            loc = glGetUniformLocation(program.ID, varName.c_str());
            glUniform1i(loc, i); // to sampler2d
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
};

// ================================================================================================= //
 GLuint loadTextureFromFile(const char *cpath, bool toLinear = true) {
    std::cout<<cpath<<std::endl;
    std::string spath = std::string(spath);

    GLuint texID=0;
    glGenTextures(1, &texID);

    int w, h, channels;
    // 0 => comp 있는대로
    void* buf = stbi_load(cpath, &w, &h, &channels, 0);
        
    if (!buf) {
        fprintf(stderr,"Texture failed to load at path: %s\n", cpath);
        stbi_image_free(buf);
        return texID;
    } else {
        fprintf(stdout,"Texture loaded : %s , %dx%d, %d channels\n", cpath, w, h, channels);
    }

    // load into vram
    GLenum format;
    switch (channels) {
        case 1: format = GL_ALPHA; break;
        case 2: format = 0; break;
        case 3: format = GL_RGB; break;
        case 4: {
            if( !toLinear) format = GL_RGBA;  
            else format = GL_SRGB8_ALPHA8; // if hdr => 10bit
        }break;
    }

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // 0~1 반복 x
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);//GL_REPEAT
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // GL_NEAREST : texel만 읽음
    // GL_LINEAR : 주변점 선형보간
    // texture을 키울때는 선형보간말고 다른방법 없음.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // GL_LINEAR_MIPMAP_LINEAR : mipmap에서 찾아서 4점을 보간하고 다른 mipmap에서 찾아서 또 섞는다.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    // level : 0 mipmap의 가장큰 level, 나머지는 알아서 생성
    // internalformat: 파일에 저장된값, format: 사용할값
    // 따라서 srgb에서 rgb로 선형공간으로 색이 이동되고 계산된후 다시 감마보정을 해준다.
    // GL_SRGB8_ALPHA8 8은 채널이 8비트 그냥은 10비트
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(buf);
    return texID;
}

struct Model {
    std::string name;
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory; // for load texture
    
private:
    // load texture
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
        std::vector<Texture> textures;
        for(GLuint i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            const char* texPath = str.C_Str(); // filename or dir
            printf("%s\n", texPath);

            // check already loaded
            // to skip loading same texture 
            bool skip = false;
            for(GLuint j = 0; j < textures_loaded.size(); j++) {
                if(std::strcmp(textures_loaded[j].path.data(), texPath)==0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            // load texture
            if(!skip) {
                Texture texture;
                std::string fullTexPath = directory+std::string(texPath);
                texture.id = loadTextureFromFile(fullTexPath.c_str(), false);
                switch(type) {
                case aiTextureType_DIFFUSE: texture.type = "map_Kd";
                case aiTextureType_SPECULAR: texture.type = "map_Ks";
                case aiTextureType_AMBIENT: texture.type = "map_Ka";
                case aiTextureType_HEIGHT: texture.type = "map_Bump";
                }
                texture.path = texPath;
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;

        // - per vertex
        for(GLuint i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            glm::vec3 v; // it can be pos, norm and uv
            // positions
            v.x = mesh->mVertices[i].x;
            v.y = mesh->mVertices[i].y;
            v.z = mesh->mVertices[i].z;
            vertex.pos = v;
            // normals
            if (mesh->HasNormals()) {
                v.x = mesh->mNormals[i].x;
                v.y = mesh->mNormals[i].y;
                v.z = mesh->mNormals[i].z;
                vertex.norm = v;
            }
            // texture coordinates
            // 버텍스당 최대 8개의 uv를 가질수있다
            if(mesh->mTextureCoords[0]) {
                glm::vec2 uv;
                uv.x = mesh->mTextureCoords[0][i].x; 
                uv.y = mesh->mTextureCoords[0][i].y;
                vertex.uv = uv;
                // tangent
                v.x = mesh->mTangents[i].x;
                v.y = mesh->mTangents[i].y;
                v.z = mesh->mTangents[i].z;
                vertex.tangent = v;
                // bitangent
                v.x = mesh->mBitangents[i].x;
                v.y = mesh->mBitangents[i].y;
                v.z = mesh->mBitangents[i].z;
                vertex.bitangent = v;
            }
            else {
                vertex.uv = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }
        
        // - per triangle
        for(GLuint i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(GLuint j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);        
        }

        // - materials. per texture type
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse maps
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 2. specular maps
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // 3. normal maps
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // 4. ambient maps
        std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        std::cout<<"mesh";
        return Mesh(vertices, indices, textures);
    }

    void processNode(aiNode *node, const aiScene *scene)
    {
        // in current node
        for(GLuint i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        for(GLuint i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }
    void cleanUp() {
    }
public :
    Model() {}

    void load(const std::string& path) {
        {
            size_t slashPos = path.find_last_of('/');
	        if( slashPos == std::string::npos ) {
                name = path;
                directory = "";
            }
            else if(slashPos == path.length()-1) {
                name = "";
                directory = path;
            }
            else {
                name = path.substr(slashPos+1);
                directory = path.substr(0, path.find_last_of('/'));
            }
        }

        Assimp::Importer loader;
        // aiProcess_Triangulate : 다각형이 있다면 삼각형으로
        // aiProcess_FlipUVs : opengl 텍스쳐 밑에서 읽는문제 or stbi_set_flip_vertically_on_load(true)
        // aiProcess_GenNormals : 노멀이 없으면 생성
        // aiProcess_SplitLargeMeshes : 큰 mesh를 작은 sub mesh로 나눠줌
        // aiProcess_OptimizeMeshes : mesh를 합쳐서 draw call을 줄인다.
        GLuint pFrags =  aiProcess_Triangulate | aiProcess_GenSmoothNormals 
                               | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
        const aiScene* scene = loader.ReadFile(path,pFrags);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE 
                  || !scene->mRootNode) {
            fprintf(stderr, "%s\n", loader.GetErrorString());
            return;
        } 

        cleanUp();

        // recursive fashion
        processNode(scene->mRootNode, scene);
        fprintf(stdout, "model %s load success\n", name.c_str());
    }

    void draw(Program &program) {
        for(GLuint i = 0; i < meshes.size(); i++)
            meshes[i].draw(program);
    }
};

#endif // !MODEL_H