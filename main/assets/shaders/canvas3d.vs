// shadow map and model view projection

#version 410 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;
// instance buffer
layout(location=3) in vec4 aMtx;
layout(location=7) in vec4 aCol;

out vec3 wPos;
out vec4 lPos; // light clip space position for shadow
out vec3 wNor;
flat out vec4 oColor;

uniform mat4 mtx_View;
uniform mat4 mtx_Proj;
uniform mat4 mtx_ShadowVp = mat4(1);

void main()
{
	wPos = vec3(aMtx*vec4(aPos, 1.f));
	//wNor = mat3(mtx_Model)*aNor;
	wNor = vec3(aMtx*vec4(aNor, 0.f));
	wNor = normalize(wNor);

	lPos = mtx_ShadowVp * vec4(wPos, 1.0);

	oColor = aCol;

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}