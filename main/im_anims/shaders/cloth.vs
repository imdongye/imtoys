#version 410

layout(location=0) in vec4 aPosm;
layout(location=1) in vec3 aNor;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

out float aM;
out vec3 wPos;
out vec3 wNor;

void main()
{
	aM = aPosm.w;
	wPos = vec3(mtx_Model*vec4(aPosm.xyz,1));
	wNor = vec3(mtx_Model*vec4(aNor.xyz,0));
	gl_Position = mtx_Proj*mtx_View*vec4(aPosm.xyz,1.f);
}