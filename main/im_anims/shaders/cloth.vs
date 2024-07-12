#version 410

layout(location=0) in vec4 aPosm;

uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

out float aM;

void main()
{
	aM = aPosm.w;
	gl_Position = mtx_Proj*mtx_View*vec4(aPosm.xyz,1.f);
}