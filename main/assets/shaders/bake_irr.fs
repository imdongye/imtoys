#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;

uniform sampler2D map_Light;


vec2 uvFromDir(vec3 v) {
	float theta = atan(v.z, v.x); // [0,2PI]->[0,1] z축이 반대방향이라 시계방향으로 뒤집힘
	float phi = asin(v.y);  	  // [1,-1]->[PI/2,-PI/2]->[0,PI]->[1,0]
    vec2 uv = vec2(theta/(2*PI), phi/PI+0.5);
    return uv;
}
vec3 dirFromUv(vec2 uv) {
	float theta = 2*PI*(uv.x);// [0,1]->[0,2PI] 
	float phi = PI*(1-uv.y);  // [0,1]->[PI,0]
	return vec3( cos(theta)*sin(phi), cos(phi), sin(theta)*sin(phi) );
}

const int nrSamples = 100;
vec3 integrateIBL( vec3 N ) {
	vec3 sum = vec3(0);
    float wsum = 0;
	for(int i=0; i<nrSamples; i++) for(int j=0; j<nrSamples; j++) {
		vec2 uv = vec2( i/float(nrSamples), j/float(nrSamples) );
		//uv = rand(uv, i); // 레귤러셈플링을 안해도 엘리어싱 안생기고 차이 없다.
        float phi = PI*(uv.y-0.5);
        vec3 L = dirFromUv(uv);
        vec3 colL = texture(map_Light, uv).rgb;
        float NDL = dot(N,L); // Todo: max0으로 가중치 0으로 처리하는게 >0분기보다 빠른지 테스트
		if(NDL>0) { // hemisphere
			sum += colL*NDL*cos(phi);
			wsum += NDL;
		}
	}
	return sum/wsum;
}

void main() {
    vec3 outColor = integrateIBL(dirFromUv(texCoord));
    FragColor = vec4(outColor, 1);
}