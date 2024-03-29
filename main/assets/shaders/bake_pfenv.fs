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
vec3 importanceSampleGGX1(vec2 Xi, vec3 N, float r){
    float a = r*r;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}  
// Rodrigues ver https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
vec3 importanceSampleGGX2(vec2 uv, vec3 N, float r) {
	float a = r*r;
	float theta = 2*PI*uv.x;
	float phi = PI*(uv.y-0.5);
	float cosPhi = sqrt( (1-uv.y)/(1+(a*a-1)*uv.y) );
	float sinPhi = sqrt( 1-cosPhi*cosPhi );
	// tangent space
	vec3 tH = vec3(sinPhi*cos(theta), sinPhi*sin(theta), cosPhi);
	// tangent space z axis to N with rotate kap
	vec3 up = abs(N.z)<0.999? vec3(0,0,1) : vec3(1,0,0);
	vec3 k = normalize(cross(up, N));
	float cosKap = N.z; // dot(N, up)
	float sinKap = length(N.xy);
	return tH*cosKap + cross(k,tH)*sinKap + k*dot(k,tH)*(1-cosKap);
}


const int nrSamplesW = 60;
const int nrSamplesH = 30;
vec3 integrateIBL( vec3 R ) {
	vec3 N = R;
	vec3 V = R; // *논문에서 V의 H에 대한 리플렉트로 L을구하는게 아니라 N으로 구한다

	vec3 sum = vec3(0);
    float wsum = 0;
	for(int i=0; i<nrSamplesW; i++) for(int j=0; j<nrSamplesH; j++) {
		vec2 uv = vec2( i/float(nrSamplesW-1), j/float(nrSamplesH-1) );
		uv = rand(uv, i); // 레귤러셈플링을 안해도 엘리어싱 안생기고 차이 없다.
        vec3 H = normalize(importanceSampleGGX2(uv, N, roughness));
        vec3 L = normalize(reflect(-V, H)); 
        // float NDL = dot(N,H); // * L대신 H??
        float NDL = dot(N,L);

		if(NDL<=0) // hemisphere
			continue;

		// Todo: 중간 두점 아티팩트 왜그런지 모르겠음
		// vec3 colL = texture(map_Light, uvFromDir(L)).rgb;
		vec3 colL = textureLod(map_Light, uvFromDir(L), sqrt(roughness)*6.8).rgb; // 교수님 아티팩트 줄이기위한 mipmap어프로치
		sum += colL*NDL; // ndf*Li의 썸이지만 임포턴스셈플링의 계수 ndf로켄슬아웃된것
		wsum += 1.0;
	}
	return sum/wsum;
}

void main() {
    vec3 outColor = integrateIBL(dirFromUv(texCoord));
    FragColor = vec4(outColor, 1);
}