#version 410
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec2 uvScale = vec2(1.f);
uniform sampler2D uvgridTex;

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
struct ShadowDirectional {
	bool Enabled;
	float ZNear;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform LightDirectional lit;
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;

void main() {
	// vec3 N = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
	vec3 L = normalize(lit.Pos-wPos);
	vec3 V = normalize(cam_Pos-wPos);
	float ndotv = max(0, dot(N, V));
	float lambertian = max(0,dot(N,L));
	vec2 scaledUv = uvScale * (mUv-vec2(.5f)) + vec2(.5f);
	vec3 outColor = vec3(0);

	// for debuging
	// outColor = vec3(lambertian);
	//outColor = texture(uvgridTex, scaledUv).rgb;
	// outColor = vec3(scaledUv, 0);
	outColor = texture(uvgridTex, scaledUv).rgb*vec3(lambertian*0.7+0.3);
	//outColor = vec3(max(0, dot(N, L)));
	//outColor = vec3(mUv, 1.0);
	//outColor = vec3(1);

	outColor = pow(outColor, vec3(1/2.2f));
	FragColor = vec4(outColor,1);
}
