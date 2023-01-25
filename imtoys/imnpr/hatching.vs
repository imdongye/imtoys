#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
	wPos = vec3(modelMat*vec4(aPos, 1.f));
	wNor = mat3(modelMat)*aNor;
	mUv = aUv;

	gl_Position = projMat*viewMat*vec4(wPos,1.f);
}