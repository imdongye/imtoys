#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	wPos = vec3(model*vec4(aPos, 1.f));
	wNor = mat3(model)*aNor;
	mUv = aUv;

	gl_Position = projection*view*vec4(wPos,1.f);
}