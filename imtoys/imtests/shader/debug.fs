#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 FragColor;

uniform float time = 0.f;
uniform vec2 uvScale = vec2(1.f);
uniform sampler2D uvgridTex;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

void main() {
	vec3 FaceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
	vec3 L = normalize(lightPos-wPos);
	float lambertian = max(0,dot(N,L));
	vec2 scaledUv = uvScale * (mUv-vec2(.5f)) + vec2(.5f);
	vec3 outColor = vec3(0);

	// for debuging
	outColor = vec3(lambertian);
	outColor = texture(uvgridTex, scaledUv).rgb;
	//outColor = texture(uvgridTex, scaledUv).rgb+vec3(lambertian);
	//outColor = vec3(max(0, dot(FaceN, L)));
	//outColor = vec3(mUv, 1.0);

	FragColor = vec4(outColor,1);
}
