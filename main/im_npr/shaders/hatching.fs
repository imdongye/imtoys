#version 410

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

layout(location=0) out vec4 FragColor;

uniform vec2 uvScale = vec2(1.f);
uniform int nrTones = 6;
uniform sampler2D tam[6];
uniform sampler2D uvgridTex;
uniform int fixedArtMapIdx = -1;
uniform bool is6way = true;

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;

void main() {
	vec3 FaceN = normalize(cross(dFdx(wPos), dFdy(wPos)));
	vec3 N = normalize(wNor);
	vec3 L = normalize(lit.Pos-wPos);
	float lambertian = max(0,dot(N,L));
	vec2 scaledUv = uvScale * (mUv-vec2(.5f)) + vec2(.5f);
	vec3 outColor = vec3(0);

	// in paper
	if(is6way) {
		int nrRange = nrTones;
		float coefs[6] = float[6](0,0,0,0,0,0);
		int left = min( int(floor( nrRange*lambertian)), nrRange );

		float coef = float(nrRange)*lambertian - float(left);
		coefs[left] = 1.f - coef;
		coefs[left+1] = coef;
		
		outColor = vec3(0);
		for( int i=0; i<nrTones; i++ ) {
			vec3 negate = vec3(1)-texture(tam[i], scaledUv).rgb;
			outColor += coefs[i] * negate;
		}
		outColor = vec3(1)-outColor;
	}
	// my way
	else {
		int nrRange = nrTones-1;
		int left = min(int(floor( nrRange*lambertian )), nrRange-1);
		float coef = float(nrRange)*lambertian - float(left);

		outColor =  mix(texture(tam[left], scaledUv), texture(tam[left+1], scaledUv), coef).rgb;
	}

	if(fixedArtMapIdx>=0) {
		outColor = texture(tam[fixedArtMapIdx], scaledUv).rgb;
		
		outColor = (1.f-lambertian)*(vec3(1)-outColor);
		outColor = vec3(1)-outColor;
	}

	// for debuging
	//outColor = N;
	//outColor = texture(tam[left], scaledUv).rgb;
	//outColor = vec3(left/float(nrTones));
	//outColor = vec3(coef);

	FragColor = vec4(outColor,1);
}
