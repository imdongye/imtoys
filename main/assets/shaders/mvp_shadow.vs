// shadow map and model view projection

#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;
out vec4 lPos; // light clip space position for shadow

uniform mat4 mtx_Model;
uniform mat4 mtx_View;
uniform mat4 mtx_Proj;
uniform mat4 mtx_ShadowVp = mat4(1);

void main()
{
	wPos = vec3(mtx_Model*vec4(aPos, 1.f));
	//wNor = mat3(mtx_Model)*aNor;
	wNor = vec3(mtx_Model*vec4(aNor, 0.f));
	wNor = normalize(wNor);
	mUv = aUv;

	lPos = mtx_ShadowVp * vec4(wPos, 1.0);

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}