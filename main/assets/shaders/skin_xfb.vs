#version 460 core
const int MAX_BONES = 200;
const int MAX_BONE_PER_VERT = 4;

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in ivec4 aBoneIdxs;
layout(location=3) in vec4 aBoneWeights;

layout(xfb_buffer=0, xfb_offset=0) out vec3 sPos;
layout(xfb_buffer=1, xfb_offset=0) out vec3 sNor;

uniform mat4 mtx_Bones[MAX_BONES];

void main()
{
    mat4 mtxBone = mat4(0.f);
    for(int i = 0; i < MAX_BONE_PER_VERT; i++) {
        if(aBoneIdxs[i] == -1) break;
        mtxBone += mtx_Bones[aBoneIdxs[i]] * aBoneWeights[i];
    }
    sPos = vec3(mtxBone * vec4(aPos, 1.f));
    sNor = vec3(mtxBone * vec4(aNor, 0.f));
    sNor = normalize(sNor);
}