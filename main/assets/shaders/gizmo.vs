
#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;

out vec3 wPos;
out vec3 wNor;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

void main()
{
	wPos = vec3(mtx_Model*vec4(aPos, 1.f));
	//wNor = mat3(mtx_Model)*aNor;
	wNor = vec3(mtx_Model*vec4(aNor, 0.f));

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}