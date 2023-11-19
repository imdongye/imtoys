#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;
layout(location=0) out vec4 FragColor;

uniform vec2 uvScale = vec2(1.f);
uniform sampler2D uvgridTex;

uniform vec3 light_Pos;
uniform vec3 light_Color;
uniform float light_Int;
uniform vec3 camera_Pos;

uniform vec3 mat_BaseColor;

void main() {
	vec3 FaceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
	vec3 L = normalize(light_Pos-wPos);
	vec3 V = normalize(camera_Pos-wPos);
	float ndotv = max(0, dot(N, V));
	float lambertian = max(0,dot(FaceN,L));
	vec2 scaledUv = uvScale * (mUv-vec2(.5f)) + vec2(.5f);
	vec3 outColor = vec3(0);

	// for debuging
	//outColor = vec3(lambertian);
	//outColor = texture(uvgridTex, scaledUv).rgb;
	outColor = texture(uvgridTex, scaledUv).rgb*vec3(lambertian*0.7+0.3);
	//outColor = vec3(max(0, dot(FaceN, L)));
	//outColor = vec3(mUv, 1.0);
	//outColor = vec3(1);

	outColor = pow(outColor, vec3(1/2.2f));
	FragColor = vec4(outColor,1);
}
