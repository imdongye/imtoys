/*

2023fall advenced rendering theorem 2 

*/
#version 410
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

const float PI = 3.1415926535;
const int MF_NONE       = 0;
const int MF_BASE_COLOR = 1<<0;
const int MF_SPECULAR   = 1<<1;
const int MF_HEIGHT     = 1<<2;
const int MF_NOR        = 1<<3;
const int MF_AMB_OCC    = 1<<4;
const int MF_ROUGHNESS  = 1<<5;
const int MF_METALNESS  = 1<<6;
const int MF_EMISSION   = 1<<7;
const int MF_Opacity    = 1<<8;
const int MF_MR         = 1<<9;
const int MF_ARM        = 1<<10;
const int MF_SHININESS  = 1<<11;

uniform vec3 baseColor;
uniform vec3 specColor;
uniform vec3 ambientColor;
uniform vec3 emissionColor;

uniform float transmission;
uniform float refraciti;
uniform float opacity;
uniform float shininess;
uniform float roughness;
uniform float metalness;
uniform vec3 f0 = vec3(1);

uniform int map_Flags;

uniform sampler2D map_BaseColor;
uniform sampler2D map_Specular;
uniform sampler2D map_Bump;
uniform sampler2D map_AmbOcc;
uniform sampler2D map_Roughness;
uniform sampler2D map_Metalness;
uniform sampler2D map_Emission;
uniform sampler2D map_Opacity;

uniform int useIBL;
uniform sampler2D map_Light;

uniform float texDelta;
uniform float bumpHeight;


uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightInt;
uniform vec3 cameraPos;

uniform int model_idx = 0;
uniform int D_idx = 0;
uniform int G_idx = 0;
uniform int F_idx = 0;

mat3 getTBN(vec3 N) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t-st2.s*st1.t;
	return mat3(normalize((Q1*st2.t-Q2*st1.t)*D),
				normalize((Q2*st1.s-Q1*st2.s)*D), N);
}

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

vec3 LambertianBRDF() { // Diffuse
	return vec3(1/PI);
}

vec3 PhongBRDF() { // Specular
	float normalizeFactor = (shininess+1)/(2*PI);
	return ambientColor + vec3(1) * normalizeFactor * pow( max(0,dot(R, w_o)), shininess );
}
vec3 BlinnPhongBRDF() { // Specular
	float normalizeFactor = (shininess+1)/(2*PI);
	return ambientColor + vec3(1) * normalizeFactor * pow( max(0,dot(N, H)), shininess );
}
vec3 OrenNayar(float r) {
	float cosTi = dot(w_i, N);
	float cosTr = dot(w_o, N);
	float thetaI = acos(dot(w_i, N));
	float thetaO = acos(cosTr);
	float cosPhi = dot( normalize(w_i-N*cosTi), normalize(w_o-N*cosTr) );
	float alpha = max(thetaI, thetaO);
	float beta = min(thetaI, thetaO);
	float aa = r*r;
	float sigmaa = aa/(aa+0.09);
	float c1 = 1-0.5*aa/(aa+0.33);
	float c2 = 0.25*sigmaa*(cosPhi>=0?sin(alpha):(sin(alpha)-pow(2*beta/PI,3)));
	float c3 = 0.125*sigmaa*pow(4*alpha*beta/(PI*PI),2);
	float l1 = cosTi*(c1+c2*cosPhi*tan(beta)+c3*(1-abs(cosPhi))*tan((alpha+beta)/2));
	float l2 = 0.17*cosTi*(aa/(aa+0.13))*(1-cosPhi*pow(2*beta/PI,2));
	return vec3(l1+l2);
}
float BlinnPhongDistribution(float r) {
	float a = r*r;
	float k = 2/(a*a)-2; // instead of Ns(Shininess)
	float normalizeFactor = 1/(PI*a*a); // for integral to 1
	return normalizeFactor * pow( max(0,dot(N, H)), k );
}
float GGX_Distribution(float r) {
	float a = r*r;
	float aa = a*a;
	float theta = acos(dot(N,H));
	return aa/( PI * pow(cos(theta),4) * pow(aa+pow(tan(theta),2),2) );
}
float BeckmannDistrobution(float r) 
{
	float a = r*r;
	float aa = max(a*a, 0.0000001);
	float cosTheta = max(dot(N, H), 0.0000001);
    float cos2Theta = cosTheta*cosTheta;
	float cos4Theta = cos2Theta*cos2Theta;
	float tan2Theta = (cos2Theta-1)/cos2Theta;
	float D = exp(tan2Theta/aa) / (PI*aa*cos4Theta);
	return D;
}
float CookTorranceGeometry() { // Cook-Torrance
	float t1 = 2*dot(N,H)*dot(N,w_o)/dot(w_o,H);
	float t2 = 2*dot(N,H)*dot(N,w_i)/dot(w_o,H);
	return min(1, min(t1, t2));
}
vec3 SchlickFresnel(vec3 F0) { // Schlick’s // 다시
	float theta = dot(N, H);
	// theta = dot(R, w_o);
	return F0+(vec3(1)-F0)*pow(1-cos(theta), 5);

	// return mix(F0, vec3(1), pow(1-max(0,dot(w_i,N)), 5));
}
// 오렌나야

vec3 CookTorranceBRDF(float roughness, vec3 F0) {
	float D, G;
	vec3 F;
	switch(D_idx) {
		case 0: D = BlinnPhongDistribution(roughness); break;
		case 1: D = GGX_Distribution(roughness); break;
		case 2: D = BeckmannDistrobution(roughness); break;
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
vec3 sampleIBL( vec3 r ) {
	float theta = atan(r.z, r.x);
	float phi = atan(r.y, length(r.xz));
	vec2 uv = vec2(1-theta/(2*PI), 0.5-phi/PI);
	return texture(map_Light, uv).rgb;
}
vec3 brdf( vec3 baseColor, float roughness, float metalness, vec3 F0 ) {
	vec3 diffuse, specular;

	diffuse = baseColor * mix(LambertianBRDF(), vec3(0), metalness);

	switch(model_idx) {
		case 0: specular = PhongBRDF(); break;
		case 1: specular = BlinnPhongBRDF(); break;
		case 2: specular = CookTorranceBRDF(roughness, F0); break;
	}
	//return sampleIBL(R);
	return diffuse+specular;
}

vec3 emission() {
	return vec3(0);
}

 

void main() {
    vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
	N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);
	//if( dot(N,faceN)<0 ) N = -N; // 모델의 내부에서 back face일때 노멀을 뒤집는다.
	vec3 toLight = lightPos-wPos;
	w_i = normalize( toLight );
	w_o = normalize( cameraPos - wPos );
	R = normalize(2*dot(N, w_i)*N-w_i);
	H = normalize((w_i+w_o)/2);

	vec3 BaseColor = baseColor;
	if( (map_Flags & MF_BASE_COLOR)>0 ) {
		BaseColor = texture( map_BaseColor, mUv ).rgb;
	}
	float AmbOcc = ambientColor.r; // todo
	if( (map_Flags & MF_ROUGHNESS)>0 ) {
		AmbOcc = texture( map_AmbOcc, mUv ).r;
	}
	float Roughness = roughness;
	if( (map_Flags & MF_ROUGHNESS)>0 ) {
		Roughness = texture( map_Roughness, mUv ).r;
	}
	float Metalness = metalness;
	if( (map_Flags & MF_METALNESS)>0 ) {
		Metalness = texture( map_Metalness, mUv ).r;
	}
	if( (map_Flags & MF_ARM)>0 ) {
		vec3 texCol = texture( map_Roughness, mUv ).rgb;
		AmbOcc 	  = texCol.r;
		Roughness = texCol.g;
		Metalness = texCol.b;
	}
	if( (map_Flags & MF_NOR)>0 ) {
		mat3 tbn = getTBN(N);
		N = tbn* (texture( map_Bump, mUv ).rgb*2 - vec3(1));
	}
	vec3 Li = lightInt*lightColor/dot(toLight,toLight); // 빛은 거리 제곱에 반비례함
	vec3 outColor = emission() + brdf( BaseColor, Roughness, Metalness, f0 ) * Li * dot(N,w_i);

	//debug
	//outColor = vec3((map_Flags & MF_Kd));
	//outColor = albedo.rgb;
	//outColor = vec3(Roughness);

	// gamma correction
	outColor.rgb = tonemap(outColor.rgb,mat3(1),2.4);

	FragColor = vec4(outColor, 1.f-opacity);
}