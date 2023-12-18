#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;

uniform sampler2D map_Light;

vec3 dirFromUv(vec2 uv) {
	float theta = 2*PI*uv.x;
	float phi = PI*(uv.y-0.5);
	return vec3( cos(theta)*cos(phi), sin(phi), sin(theta)*cos(phi) );
}

const int nrSamples = 100;
vec3 integrateIBL( vec3 N ) {
	vec3 sum = vec3(0);
    float wsum = 0;
	for(int i=0; i<nrSamples; i++) for(int j=0; j<nrSamples; j++) {
		vec2 uv = vec2( i/float(nrSamples), j/float(nrSamples) ); // Todo 레귤러렌덤
        float phi = PI*(uv.y-0.5);
        vec3 L = dirFromUv(uv);
        vec3 colL = texture(map_Light, uv).rgb;
        float NDL = max(0,dot(N,L)); // hemisphere
        wsum += NDL;
		sum += colL*NDL*cos(phi);
	}
	return sum/wsum;
}

void main() {
    vec3 outColor = integrateIBL(dirFromUv(texCoord));
    FragColor = vec4(outColor, 1);
}