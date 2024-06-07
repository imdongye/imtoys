#version 410
const int MAX_BONES = 100;
const int MAX_BONE_PER_VERT = 4;

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;
layout(location=6) in ivec4 aBoneIds;
layout(location=7) in vec4 aBoneWeights;

flat out ivec4 oBoneIds;
out vec4 oBoneWieghts;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;
uniform mat4 mtx_Bones[MAX_BONES];




void main()
{
    oBoneIds = aBoneIds;
    oBoneWieghts = aBoneWeights;

    vec4 weightedPos = vec4(0.f);
    vec4 weightedNor = vec4(0.f);
    for(int i = 0; i < MAX_BONE_PER_VERT; i++) {
        int id = aBoneIds[i];
        if(id == -1)
            continue;
        if(id >= MAX_BONES) {
            weightedPos = vec4(aPos, 1.f);
            weightedNor = vec4(aNor, 0.f);
            break;
        }
        vec4 localPos = mtx_Bones[id] * vec4(aPos, 1.f);
        weightedPos += localPos * aBoneWeights[i];
        vec4 localNor = mtx_Bones[id] * vec4(aNor, 0.f);
        weightedNor += localNor * aBoneWeights[i];
    }
    weightedPos = vec4(aPos, 1.f);
    weightedNor = vec4(aNor, 0.f);

    wPos = vec3(mtx_Model*weightedPos);
	wNor = vec3(mtx_Model*weightedNor);
	wNor = normalize(wNor);
	mUv = aUv;

	gl_Position = mtx_Proj*mtx_View*vec4(wPos, 1.f);
}