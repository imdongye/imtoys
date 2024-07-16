#version 460 core

layout(location=0) in vec4 aPos;

uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

void main()
{
	gl_Position = mtx_Proj*mtx_View*vec4(aPos.xyz,1.f);
}