// shadow map view projection

#version 410

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;
layout(location=2) in vec2 aUv;

out vec3 wPos;
out vec3 wNor;
out vec2 mUv;
out vec4 lPos; // light unit space position for shadow

uniform mat4 model_Mat;
uniform mat4 view_Mat;
uniform mat4 proj_Mat;
uniform mat4 shadow_VP = mat4(1);


void main()
{
	wPos = vec3(model_Mat*vec4(aPos, 1.f));
	//wNor = mat3(model_Mat)*aNor;
	wNor = vec3(model_Mat*vec4(aNor, 0.f));
	wNor = normalize(wNor);
	mUv = aUv;

	lPos = shadow_VP * vec4(wPos, 1.0);

	gl_Position = proj_Mat*view_Mat*vec4(wPos,1.f);
}