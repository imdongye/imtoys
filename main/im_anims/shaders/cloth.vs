#version 410

layout(location=0) in vec4 aPosm;

out vec3 wPos;

uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

void main()
{
	wPos = aPosm.xyz;

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}