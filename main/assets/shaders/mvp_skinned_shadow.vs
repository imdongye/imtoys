#version 460 core
const int MAX_BONES = 200;
const int MAX_BONE_PER_VERT = 4;

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;
layout(location=2) in ivec4 aBoneIdxs;
layout(location=3) in vec4 aBoneWeights;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;
out vec4 lPos;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;
uniform mat4 mtx_ShadowVp = mat4(1);
uniform bool is_Skinned;
uniform mat4 mtx_Bones[MAX_BONES];

void main()
{
    if( is_Skinned ) {
        mat4 mtxBone = mat4(0.f);
        for(int i = 0; i < MAX_BONE_PER_VERT; i++) {
            if(aBoneIdxs[i] == -1) break;
            mtxBone += mtx_Bones[aBoneIdxs[i]] * aBoneWeights[i];
        }
        mat4 mtxFinal = mtx_Model * mtxBone;
        wPos = vec3(mtxFinal * vec4(aPos, 1.f));
        wNor = vec3(mtxFinal * vec4(aNor, 0.f));
    } else {
        wPos = vec3(mtx_Model*vec4(aPos, 1.f));
        wNor = vec3(mtx_Model*vec4(aNor, 0.f));
    }
    wNor = normalize(wNor);
	mUv = aUv;
	lPos = mtx_ShadowVp * vec4(wPos, 1.0);
    gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}