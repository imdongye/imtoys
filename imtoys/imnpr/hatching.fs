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

		// for debuging
		//outColor = vec3(test);
		//outColor = vec3(left/float(nrTones));
		//outColor = vec3(coefs[3]);
	}
	else {
		int nrRange = nrTones-1;
		int left = min(int(floor( nrRange*lambertian )), nrRange-1);
		float coef = float(nrRange)*lambertian - float(left);

		outColor =  mix(texture(tam[left], scaledUv), texture(tam[left+1], scaledUv), coef).rgb;

		// for debuging
		//outColor = vec3(coef);
	}

	if(fixedArtMapIdx>=0) {
		outColor = texture(tam[fixedArtMapIdx], scaledUv).rgb;
		
		outColor = vec3(1)-outColor;
		outColor *= 1.f-lambertian;
		outColor = vec3(1)-outColor;
	}

	// for debuging
	//outColor = N;
	//outColor = texture(tam[left], scaledUv).rgb;
	//outColor = vec3(left/float(nrTones));
	//outColor = vec3(coef);
	//outColor = vec3(lambertian);
	//outColor = vec3(mUv, 1);
	outColor = texture(uvgridTex, scaledUv).rgb;
	outColor = vec3(max(0, dot(FaceN, L)));
	//outColor = vec3(mUv, 1.0);

	FragColor = vec4(outColor,1);
}
