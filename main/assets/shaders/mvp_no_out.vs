#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

void main()
{
	gl_Position = mtx_Proj * mtx_View * mtx_Model * vec4(aPos,1.f);
}