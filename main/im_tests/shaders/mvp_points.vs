#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;

void main()
{
	wPos = vec3(mtx_Model*vec4(aPos, 1.f));
	wNor = mat3(mtx_Model)*aNor;
	mUv = aUv;

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
	gl_PointSize = 2.f;
}