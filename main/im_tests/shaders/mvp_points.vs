#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 model_Mat;
uniform mat4 view_Mat;
uniform mat4 proj_Mat;

void main()
{
	wPos = vec3(model_Mat*vec4(aPos, 1.f));
	wNor = mat3(model_Mat)*aNor;
	mUv = aUv;

	gl_Position = proj_Mat*view_Mat*vec4(wPos,1.f);
	gl_PointSize = 2.f;
}