// shadow map and model view projection

#version 460 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec4 lPos; // light clip space position for shadow
out vec3 wNor;
out vec4 oColor;

struct PrimInfo {
	mat4 mtx;
	vec4 col;
};
layout(std430, binding=0) buffer buf_prims {
    PrimInfo prims[];
};

uniform mat4 mtx_View;
uniform mat4 mtx_Proj;
uniform mat4 mtx_ShadowVp = mat4(1);

void main()
{
	PrimInfo prim = prims[gl_InstanceID];
	wPos = vec3(prim.mtx*vec4(aPos, 1.f));
	//wNor = mat3(mtx_Model)*aNor;
	wNor = vec3(prim.mtx*vec4(aNor, 0.f));
	wNor = normalize(wNor);

	lPos = mtx_ShadowVp * vec4(wPos, 1.0);

	oColor = prim.col;

	gl_Position = mtx_Proj*mtx_View*vec4(wPos,1.f);
}