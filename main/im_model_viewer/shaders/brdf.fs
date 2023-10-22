/*

2023fall advenced rendering theorem 2 

*/
#version 410
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform vec3 Tf;

uniform float d;
uniform float Tr;
uniform float Ns;
uniform float Ni;
uniform float roughness;
uniform vec3 F0 = vec3(1);

uniform int model_idx = 0;
uniform int D_idx = 0;
uniform int G_idx = 0;
uniform int F_idx = 0;

uniform int map_Flags;

uniform sampler2D map_Kd;
uniform sampler2D map_Ks;
uniform sampler2D map_Ka;
uniform sampler2D map_Ns;
uniform sampler2D map_Bump;
uniform float texDelta;
uniform float bumpHeight;

const float PI = 3.1415926535;
const int MF_None   = 0;
const int MF_Kd     = 1<<0;
const int MF_Ks     = 1<<1;
const int MF_Ka     = 1<<2;
const int MF_Ns     = 1<<3;
const int MF_Height = 1<<4;
const int MF_Nor    = 1<<5;


//***************************************************
//            Color Space Conversion Functions
//***************************************************
float inverseTonemap_sRGB(float u) { 
	float u_ = abs(u);
	return u_>0.04045?(sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);
}
float tonemap_sRGB(float u) {
	float u_ = abs(u);
	return  u_>0.0031308?( sign(u)*1.055*pow( u_,0.41667)-0.055):(12.92*u);
}
// linearization
vec3 inverseTonemap( vec3 rgb, mat3 csc, float gamma ){
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return csc*vec3( inverseTonemap_sRGB(rgb.r), inverseTonemap_sRGB(rgb.g), inverseTonemap_sRGB(rgb.b) );
	return csc*sign(rgb)*pow( abs(rgb), vec3(gamma) );
}
// gamma correction
vec3 tonemap( vec3 rgb, mat3 csc, float gamma ){
	vec3 rgb_ = csc*rgb;
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return vec3( tonemap_sRGB(rgb_.r), tonemap_sRGB(rgb_.g), tonemap_sRGB(rgb_.b) );
	return sign(rgb_)*pow( abs(rgb_), vec3(1./gamma) );
}


//*****************************************
//            Rendering Equation
//*****************************************
vec3 N, w_i, w_o, R, H;


//vec3 BlinnPhongBRDF( vec3)

vec3 LambertianBRDF() { // Diffuse
	return vec3(1/PI);
}

vec3 PhongBRDF() { // Specular
	float normalizeFactor = (Ns+1)/(2*PI);
	return vec3(1) * normalizeFactor * pow( max(0,dot(R, w_o)), Ns );
}
vec3 BlinnPhongBRDF() { // Specular
	float normalizeFactor = (Ns+1)/(2*PI);
	return vec3(1) * normalizeFactor * pow( max(0,dot(N, H)), Ns );
}
float BlinnPhongDistribution(float r) {
	float a = r*r;
	float k = 2/(a*a)-2; // instead of Ns(Shininess)
	float normalizeFactor = 1/(PI*a*a); // for integral to 1
	return normalizeFactor * pow( max(0,dot(N, H)), k );
}
float GGX_Distribution(float r) { // GGX
	float a = r*r;
	float aa = a*a;
	float theta = acos(dot(N,H));
	return aa/( PI * pow(cos(theta),4) * pow(aa+pow(tan(theta),2),2) );
}
float CookTorranceGeometry() { // Cook-Torrance
	float t1 = 2*dot(N,H)*dot(N,w_o)/dot(w_o,H);
	float t2 = 2*dot(N,H)*dot(N,w_i)/dot(w_o,H);
	return min(1, min(t1, t2));
}
vec3 SchlickFresnel(vec3 F0) { // Schlick’s
	float theta = dot(N, H);
	// theta = dot(R, w_o);
	return F0+(vec3(1)-F0)*pow(1-cos(theta), 5);
}
vec3 CookTorranceBRDF() {
	float D, G;
	vec3 F;
	switch(D_idx) {
		case 0: D = BlinnPhongDistribution(roughness); break;
		case 1: D = GGX_Distribution(roughness); break;
	}
	switch(G_idx) {
		case 0: G = CookTorranceGeometry(); break;
	}
	switch(F_idx) {
		case 0: F = SchlickFresnel(F0); break;
	}
	//return F / (4*dot(w_i,N)*dot(w_o,N));
	return D*G*F / (4*dot(w_i,N)*dot(w_o,N));
}
vec3 brdf() {
	vec3 diffuse, specular;
	diffuse = LambertianBRDF();
	switch(model_idx) {
		case 0: specular = PhongBRDF(); break;
		case 1: specular = BlinnPhongBRDF(); break;
		case 2: specular = CookTorranceBRDF(); break;
	}
	return diffuse+specular;
}

vec3 emission() {
	return vec3(0);
}

 

void main() {
    vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
	N = normalize(wNor);
	if( dot(N,faceN)<0 ) N = -N; // 모델의 내부에서 back face일때 노멀을 뒤집는다.
	vec3 toLight = lightPos-wPos;
	w_i = normalize( toLight );
	w_o = normalize( cameraPos - wPos );
	R = normalize(2*dot(N, w_i)*N-w_i);
	H = normalize((w_i+w_o)/2);

	vec4 albedo = vec4(Kd, d);
	if( (map_Flags & MF_Kd)>0 ) {
		albedo = texture( map_Kd, mUv );
	}
	vec3 Li = lightInt*lightColor/dot(toLight,toLight); // 빛은 거리 제곱에 반비례함
	vec3 outColor = emission() + brdf() * albedo.rgb * Li * dot(N,w_i);

	//debug
	//outColor = vec3((map_Flags & MF_Kd));
	//outColor = albedo.rgb;

	// gamma correction
	outColor.rgb = tonemap(outColor.rgb,mat3(1),2.4);

	FragColor = vec4(outColor, albedo.a);
}