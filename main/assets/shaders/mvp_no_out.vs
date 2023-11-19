#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

uniform mat4 model_Mat;
uniform mat4 view_Mat;
uniform mat4 proj_Mat;

void main()
{
	gl_Position = proj_Mat * view_Mat * vec4(aPos,1.f);
}