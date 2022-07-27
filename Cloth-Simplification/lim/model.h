//
//  2022-07-20 / im dong ye
//  edit learnopengl code
//
// texture uniform sampler2d variable name rule
// map_Kd0, map_Kd1 ...
// 
//  TODO list:
//  1. rigging
//  2. cleanUp
//

#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <string>
#include <functional>
#include <vector>

#include "program.h"

#define MAX_BONE_INFLUENCE 4

GLuint loadTextureFromFile(const char *cpath, bool toLinear = true) {
    std::string spath = std::string(spath);
    fprintf(stdout,"loading texture %s.\n", cpath);

    GLuint texID=0;
    glGenTextures(1, &texID);

    int w, h, channels;
    // 0 => comp 있는대로
    void* buf = stbi_load(cpath, &w, &h, &channels, 4); // todo: 0
        
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

// 4byte * (3+3+2+3+3+4+4) = 88
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

class Mesh {
public:
    const char* name;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<Texture> textures;
    GLuint angles; // set size of indices
private:
    GLuint VAO, VBO, EBO;
    GLenum drawMode;
private:
    // disable copying
    Mesh(Mesh const &) = delete;
    Mesh & operator=(Mesh const &) = delete;
public:
    Mesh(const char* _name="", const GLuint& _angle=3)
        : VAO(0), name(_name), angles(_angle)
    {
        // todo: apply every face diff draw mode
        switch( angles ) {
        case 3: drawMode = GL_TRIANGLES; break;
        case 2: drawMode = GL_LINE_STRIP; break;
        case 4: drawMode = GL_TRIANGLE_FAN; break;
        }
    }
    ~Mesh() { glDeleteVertexArrays(1, &VAO); }
    Mesh(const std::vector<Vertex>& _vertices
        , const std::vector<GLuint>& _indices
        , const std::vector<Texture>& _textures
        , const char* _name="", const GLuint& _angle=3)
        : Mesh(_name, _angle)
    {
        vertices = _vertices; // todo: fix deep copy
        indices = _indices;
        textures = _textures;
        setupMesh();
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
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); // need?
        glDrawElements(drawMode, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glActiveTexture(GL_TEXTURE0);
    }
    void cleanUp() {
        if(VAO!=0) {
            glDeleteVertexArrays(1, &VAO);
            VAO=0;
        }
    }
private:
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
		glVertexAttribIPointer(5, MAX_BONE_INFLUENCE, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
		// - weights
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

        glBindVertexArray(0);
        
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

class Model {
public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;
    glm::mat4 modelMat;
    std::string name;
private:
    std::vector<Texture> textures_loaded;
    std::vector<std::unique_ptr<Mesh>> meshes;// unique ptr makes clear auto
    std::string directory; // for load texture
private:
    // Disable Copying and Assignment
    Model(Model const &) = delete;
    Model & operator=(Model const &) = delete;
public :
    Model() : position(glm::vec3(0)), rotation(glm::quat()), scale(glm::vec3(1))
    {
        updateModelMat();
    }
    Model(const char* path) : Model() { load(path); }// todo: string_view
    // 왜 외부에서 외부에서 생성한 mesh객체의 unique_ptr을 우측값 참조로 받아 push_back할 수 없는거지
    Model(std::function<void(std::vector<Vertex>& _vertices
                            , std::vector<GLuint>& _indices
                            , std::vector<Texture>& _textures)> genMeshFunc
        , const char* _name ="")
        :Model()
    {
        name = std::string(_name);
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;
        genMeshFunc(vertices, indices, textures);

        meshes.push_back(std::make_unique<Mesh>(vertices, indices, textures, _name));

        fprintf(stdout, "\ngen model mesh : %s, v%d\n", _name, meshes.back()->vertices.size());
    }
    void cleanUp() {
        for(auto& mesh : meshes) {
            mesh->cleanUp();
        }
        meshes.clear();
    }
    void load(const char* _path) {
        cleanUp();
        std::string path = std::string(_path);
        std::replace(path.begin(), path.end(), '\\', '/');
        printf("%s", path.c_str());
        size_t slashPos = path.find_last_of('/');
	    if( slashPos == std::string::npos ) {
            name = path;
            name = name.substr(0, name.find_last_of('.'));
            directory = "";
        }
        else if(slashPos == path.length()-1) {
            name = "";
            directory = path;
        }
        else {
            name = path.substr(slashPos+1);
            name = name.substr(0, name.find_last_of('.'));
            directory = path.substr(0, path.find_last_of('/'))+"/";
        }

        fprintf(stdout, "\nmodel loading : %s\n", name.c_str());
        Assimp::Importer loader;
        // aiProcess_Triangulate : 다각형이 있다면 삼각형으로
        // aiProcess_FlipUVs : opengl 텍스쳐 밑에서 읽는문제 or stbi_set_flip_vertically_on_load(true)
        // aiProcess_GenNormals : 노멀이 없으면 생성
        // aiProcess_SplitLargeMeshes : 큰 mesh를 작은 sub mesh로 나눠줌
        // aiProcess_OptimizeMeshes : mesh를 합쳐서 draw call을 줄인다.
        GLuint pFrags =  aiProcess_Triangulate | aiProcess_GenSmoothNormals 
                               | aiProcess_FlipUVs | aiProcess_CalcTangentSpace;
        const aiScene* scene = loader.ReadFile(path, pFrags);
        
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE 
                  || !scene->mRootNode) {
            fprintf(stderr, "%s\n", loader.GetErrorString());
            return;
        } 

        // recursive fashion
        parseNode(scene->mRootNode, scene);
        fprintf(stdout, "model loaded : %s\n", name.c_str());
    }
    void draw(Program &program) {
        GLuint loc = glGetAttribLocation(program.ID, "modelMat");
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(modelMat));
        
        for(GLuint i = 0; i < meshes.size(); i++)
            meshes[i]->draw(program);
    }
    void updateModelMat() {
        
        glm::mat4 translateMat = glm::translate(position);
        glm::mat4 scaleMat = glm::scale(scale);
        glm::mat4 rotateMat = glm::toMat4(rotation);
        modelMat = translateMat * rotateMat * scaleMat;
    }
private:
    // load texture
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type) {
        std::vector<Texture> textures;
        for(GLuint i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            const char* texPath = str.C_Str(); 

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

                texture.id = loadTextureFromFile(fullTexPath.c_str(), true);

                switch(type) {
                case aiTextureType_DIFFUSE: texture.type = "map_Kd"; break;
                case aiTextureType_SPECULAR: texture.type = "map_Ks"; break;
                case aiTextureType_AMBIENT: texture.type = "map_Ka"; break;
                case aiTextureType_HEIGHT: texture.type = "map_Bump"; break;
                }
                texture.path = texPath;
                textures.push_back(texture);
                textures_loaded.push_back(texture);  // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
            }
        }
        return textures;
    }
    void parseMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;
        std::vector<Texture> textures;
        GLuint angles = 3;

        // - per vertex
        Vertex vertex;
        for(GLuint i = 0; i < mesh->mNumVertices; i++) {
            vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (mesh->HasNormals()) {
                vertex.norm = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }
            // 버텍스당 최대 8개의 uv를 가질수있지만 하나만.
            if(mesh->mTextureCoords[0]) {
                vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x,  mesh->mTextureCoords[0][i].y);
                // tangent
                vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                // bitangent
                vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }
            else {
                vertex.uv = glm::vec2(0.0f);
            }
            vertices.push_back(vertex);
        }
        
        // - per triangles
        angles = mesh->mFaces[0].mNumIndices;
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

        fprintf(stdout,"mesh loaded : %s, set%d, verts%d\n", mesh->mName.C_Str(), angles, vertices.size());

        // c++
        //std::unique_ptr<Mesh>(new Mesh(vertices, indices, textures, mesh->mName.C_Str(), angles));
        // c++ 14
        meshes.push_back(std::make_unique<Mesh>(vertices, indices, textures
                                               , mesh->mName.C_Str(), angles));
    }
    void parseNode(aiNode *node, const aiScene *scene)
    {
        // in current node
        for(GLuint i = 0; i < node->mNumMeshes; i++) {
            parseMesh(scene->mMeshes[node->mMeshes[i]], scene);
        }
        for(GLuint i = 0; i < node->mNumChildren; i++) {
            parseNode(node->mChildren[i], scene);
        }
    }
};

#endif // !MODEL_H
