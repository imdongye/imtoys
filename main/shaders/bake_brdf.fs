/*

X,Y : ndv(cos), roughness
A,B : F0계수, 프레넬쉴릭텀

*/
#version 410 core
layout (location = 0) out vec4 FragColor;

in vec2 texCoord;

const float PI = 3.1415926535;



vec3 importanceSampleGGX(vec2 uv, vec3 N, float roughness) {
	float a = roughness*roughness;
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

float _geometryShlickGGX(float num, float roughness) {
	float r = roughness+1;
	float k = r*r/8.0;
	float denom = num*(1-k)+k;
	return num/denom;
}
float geometrySmith(float NDV, float NDL, float roughness) {
	float ggx2 = _geometryShlickGGX(max(NDV, 0), roughness);
	float ggx1 = _geometryShlickGGX(max(NDL, 0), roughness);
	return ggx1*ggx2;
}
float X(float v) {
	return (v>=0)? 1.0 : 0.0;
}
const int nrSamples = 30;
vec2 integrateIBL( float NDV, float roughness ) {
	float a = roughness*roughness;
	vec3 N = vec3(0,0,1);
	vec3 V = vec3( sqrt(1.0-NDV*NDV), 0, NDV );
	vec2 sum = vec2(0);

	for(int i=0; i<nrSamples; i++) for(int j=0; j<nrSamples; j++) {
		vec2 uv = vec2( i/float(nrSamples-1), j/float(nrSamples-1) );
        vec3 H = importanceSampleGGX(uv, N, roughness);
        vec3 L = reflect(-V, H); 

		float NDL = L.z;
		float NDH = H.z;
		float VDH = dot(V,H);
	
		if(NDL>0) { // hemisphere
			// float G = geometrySmith(NDV, NDL, roughness);
			float tn2 = (1-NDV*NDV)/(NDV*NDV);
			float G = X(VDH/NDV) * (2/(1+sqrt(1+a*a*tn2)));
			float G_Vis = G*VDH/(NDH*NDV);
			float Fc = pow(1-VDH, 5);
			sum += vec2(1.0-Fc, Fc)*G_Vis;
		}
	}
	return sum/(nrSamples*nrSamples);
}

void main() {
    vec2 outColor = integrateIBL(texCoord.x, texCoord.y);
    FragColor = vec4(outColor, 0, 1);
}