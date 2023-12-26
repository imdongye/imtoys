#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;

uniform float roughness;
uniform sampler2D map_Light;

vec2 rand(vec2 st, int i) {
    return vec2( 
		fract(sin(dot(st+i*4.1233, vec2(12.9898, 78.2336))) * 43758.5453),
    	fract(sin(dot(st+i*6.8233, vec2(39.8793, 83.2402))) * 22209.2896)
	);
}
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


vec3 importanceSampleGGX(vec2 uv, vec3 N, float r) {
	float a = r*r;
	float theta = 2*PI*uv.x;
	float cosPhi = sqrt( (1-uv.y)/(1+(a*a-1)*uv.y) );
	float sinPhi = sqrt( 1-cosPhi*cosPhi );
	// tangent space
	vec3 tH = vec3(sinPhi*cos(theta), sinPhi*sin(theta), cosPhi);
	// world space
	vec3 up = abs(N.z)<0.999? vec3(0,0,1) : vec3(1,0,0);
	vec3 tanX = normalize(cross(up, N));
	vec3 tanY = cross(N, tanX);
	return tanX*tH.x + tanY*tH.y + N*tH.z;
}

const int nrSamples = 50;
vec3 integrateIBL( vec3 R ) {
	vec3 N = R;
	vec3 V = R; // *논문에서 V의 H에 대한 리플렉트로 L을구하는게 아니라 N으로 구한다

	vec3 sum = vec3(0);
    float wsum = 0;
	for(int i=0; i<nrSamples; i++) for(int j=0; j<nrSamples; j++) {
		vec2 uv = vec2( i/float(nrSamples-1), j/float(nrSamples-1) );
		// uv = rand(uv, i); // 레귤러셈플링을 안해도 엘리어싱 안생기고 차이 없다.
        vec3 H = importanceSampleGGX(uv, N, roughness);
        vec3 L = reflect(-V, H); 
        // vec3 colL = texture(map_Light, uvFromDir(L)).rgb;
        vec3 colL = texture(map_Light, uvFromDir(L), sqrt(roughness)*7).rgb; // 교수님 아티팩트 줄이기위한 mipmap어프로치
		// Todo: 중간 두점 아티팩트 왜그런지 모르겠음

        float phi = PI*(uv.y-0.5);
        float NDL = dot(N,L);
        // float NDL = dot(N,H); // * L대신 H??
		if(NDL>0) { // hemisphere
			float w = NDL*cos(phi); // 솔리드 엥글 고려안해도되나?
			sum += colL*w; // ndf*Li의 썸이지만 임포턴스셈플링의 계수 ndf로켄슬아웃된것
			wsum += w;
		}
	}
	return sum/wsum;
}

void main() {
    vec3 outColor = integrateIBL(dirFromUv(texCoord));
    FragColor = vec4(outColor, 1);
}