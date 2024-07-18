#version 410
layout(location=0) out vec4 FragColor;


struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;

struct ShadowDirectional {
	bool Enabled;
	float ZNear;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;

uniform vec3 cam_Pos;

in float aM;
in vec3 wPos;
in vec3 wNor;

void main() {
	// vec3 N = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);
	vec3 V = normalize(cam_Pos-wPos);

	vec3 color = mix(vec3(1,0.2,0.2), vec3(0.3,1,0.2), aM);
	color *= dot(N,V);

	FragColor = vec4(color, 1);
}
