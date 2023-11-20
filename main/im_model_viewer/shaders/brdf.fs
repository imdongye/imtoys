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
const int MF_OPACITY    = 1<<8;
const int MF_MR         = 1<<9;
const int MF_ARM        = 1<<10;
const int MF_SHININESS  = 1<<11;

uniform vec3 mat_BaseColor;
uniform vec3 mat_SpecColor;
uniform vec3 mat_AmbientColor;
uniform vec3 mat_EmissionColor;
uniform vec3 mat_F0;

uniform float mat_Transmission;
uniform float mat_Refraciti;
uniform float mat_Opacity;
uniform float mat_Shininess;
uniform float mat_Roughness;
uniform float mat_Metalness;
uniform float mat_TexDelta;
uniform float mat_BumpHeight;

uniform int map_Flags;

uniform sampler2D map_BaseColor;
uniform sampler2D map_Specular;
uniform sampler2D map_Bump;
uniform sampler2D map_AmbOcc;
uniform sampler2D map_Roughness;
uniform sampler2D map_Metalness;
uniform sampler2D map_Emission;
uniform sampler2D map_Opacity;

uniform int use_IBL;
uniform sampler2D map_Light;


uniform vec3 light_Pos;
uniform vec3 light_Color;
uniform float light_Int;
uniform vec3 camera_Pos;

uniform int idx_Brdf = 0;
uniform int idx_D = 0;
uniform int idx_G = 0;
uniform int idx_F = 0;


// gamma correction
void convertLinearToSRGB( inout vec3 rgb ){
    rgb.r = rgb.r<0.0031308?(12.92*rgb.r):(1.055*pow(rgb.r,0.41667)-0.055);
    rgb.g = rgb.g<0.0031308?(12.92*rgb.g):(1.055*pow(rgb.g,0.41667)-0.055);
    rgb.b = rgb.b<0.0031308?(12.92*rgb.b):(1.055*pow(rgb.b,0.41667)-0.055);
}
mat3 getTBN(vec3 N) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t-st2.s*st1.t;
	return mat3(normalize((Q1*st2.t-Q2*st1.t)*D),
				normalize((Q2*st1.s-Q1*st2.s)*D), N);
}


//*****************************************
//            Rendering Equation
//*****************************************
vec3 N, L, V, R, H;
float NDL, NDV, NDH, VDR, VDH;
vec3 baseColor, F0, emission;
float roughness, metalness, ambOcc;

vec3 brdfLambertian() { // Diffuse
	return vec3(1/PI);
}
vec3 brdfPhong() { // Specular
	float normalizeFactor = (mat_Shininess+1)/(2*PI);
	return mat_AmbientColor + vec3(1) * normalizeFactor * pow( VDR, mat_Shininess );
}
vec3 brdfBlinnPhong() { // Specular
	// vec3 H = (V+L)/2; // 성능을위해 노멀라이즈 하지 않음.
	// float NDH = max(0,dot(N, H));
	float normalizeFactor = (mat_Shininess+1)/(2*PI);
	return mat_AmbientColor + vec3(1) * normalizeFactor * pow( NDH, mat_Shininess );
}

vec3 brdfOrenNayar() {
	float a = roughness*roughness;
	float aa = a*a;
	float cosTi = dot(L, N);
	float cosTr = dot(V, N);
	float thetaI = acos(dot(L, N));
	float thetaO = acos(cosTr);
	float cosPhi = dot( normalize(L-N*cosTi), normalize(V-N*cosTr) );
	float alpha = max(thetaI, thetaO);
	float beta = min(thetaI, thetaO);
	float sigmaa = aa/(aa+0.09);
	float c1 = 1-0.5*aa/(aa+0.33);
	float c2 = 0.25*sigmaa*(cosPhi>=0?sin(alpha):(sin(alpha)-pow(2*beta/PI,3)));
	float c3 = 0.125*sigmaa*pow(4*alpha*beta/(PI*PI),2);
	float l1 = cosTi*(c1+c2*cosPhi*tan(beta)+c3*(1-abs(cosPhi))*tan((alpha+beta)/2));
	float l2 = 0.17*cosTi*(aa/(aa+0.13))*(1-cosPhi*pow(2*beta/PI,2));
	return vec3(l1+l2);
}

float distributionBlinnPhong() {
	float a = roughness*roughness;
	float k = 2/(a*a)-2; // instead of Ns(Shininess)
	float normalizeFactor = 1/(PI*a*a); // for integral to 1
	return normalizeFactor * pow( NDH, k );
}
float distributionGGX() { // Todo: 하이라이트 중간 깨짐, 음수
	float a = roughness*roughness;
	float aa = a*a;
	float theta = acos( NDH );
	float denom = PI * pow(NDH, 4)* pow(aa+pow(tan(theta),2) ,2);
	return aa/max(denom,0.00001);
}
float distributionBeckmann() 
{
	float a = roughness*roughness;
	float aa = max(a*a, 0.0000001);
	float cosTheta = NDH;
    float cos2Theta = cosTheta*cosTheta;
	float cos4Theta = cos2Theta*cos2Theta;
	float tan2Theta = (cos2Theta-1)/cos2Theta;
	float D = exp(tan2Theta/aa) / (PI*aa*cos4Theta);
	return D;
}

float geometryCookTorrance() { // GGX
	float t1 = 2*NDH*NDV/VDH;
	float t2 = 2*NDH*NDL/VDH;
	return min(1, min(t1, t2));
}

vec3 fresnelSchlick() { // Schlick’s // 다시
	float ratio = max(0,pow(1-NDV, 5));
	return mix(F0, vec3(1), ratio);
}

vec3 brdfCookTorrance() {
	float D, G;
	vec3 F;
	switch(idx_D) {
		case 0: D = distributionBlinnPhong(); break;
		case 1: D = distributionGGX(); break;
		case 2: D = distributionBeckmann(); break;
	}
	switch(idx_G) {
		case 0: G = geometryCookTorrance(); break;
	}
	switch(idx_F) {
		case 0: F = fresnelSchlick(); break;
	}
	return D*G*F / (4*NDL*NDV); // 내적 max하면 음수??
}
vec3 sampleIBL() {
	float theta = atan(R.z, R.x);
	float phi = atan(R.y, length(R.xz));
	vec2 uv = vec2(1-theta/(2*PI), 0.5-phi/PI);
	return texture(map_Light, uv).rgb;
}
vec3 brdf() {
	vec3 diffuse, specular;
	if(idx_Brdf == 3) {
		diffuse = baseColor*brdfLambertian()*brdfOrenNayar();
	} else {
		diffuse = baseColor * mix(brdfLambertian(), vec3(0), metalness);
	}

	switch(idx_Brdf) {
		case 0: specular = brdfPhong(); break;
		case 1: specular = brdfBlinnPhong(); break;
		case 2: specular = brdfCookTorrance(); break;
		case 3: specular = vec3(0); break;
	}
	//return sampleIBL(R);
	return diffuse+specular;
}


void main() {
	// vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
	// if( dot(N,faceN)<0 ) N = -N; // 모델의 내부에서 back face일때 노멀을 뒤집는다.
	N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);

	baseColor = mat_BaseColor;
	if( (map_Flags & MF_BASE_COLOR)>0 ) {
		baseColor = texture( map_BaseColor, mUv ).rgb;
	}
	ambOcc = 1.f;
	if( (map_Flags & MF_AMB_OCC)>0 ) {
		ambOcc = texture( map_AmbOcc, mUv ).r;
	}
	roughness = mat_Roughness;
	if( (map_Flags & MF_ROUGHNESS)>0 ) {
		roughness = texture( map_Roughness, mUv ).r;
	}
	metalness = mat_Metalness;
	if( (map_Flags & MF_METALNESS)>0 ) {
		metalness = texture( map_Metalness, mUv ).r;
	}
	emission = mat_EmissionColor;
	if( (map_Flags & MF_EMISSION)>0 ) {
		emission = texture( map_Emission, mUv ).rgb;
	}
	if( (map_Flags & MF_ARM)>0 ) {
		vec3 texCol = texture( map_Roughness, mUv ).rgb;
		ambOcc 	  = texCol.r;
		roughness = texCol.g;
		metalness = texCol.b;
	}
	if( (map_Flags & MF_NOR)>0 ) {
		mat3 tbn = getTBN(N);
		N = tbn* (texture( map_Bump, mUv ).rgb*2 - vec3(1));
	}
    F0 = mix(mat_F0, baseColor, metalness);

	vec3 toLight = light_Pos-wPos;
	L = normalize( toLight );
	V = normalize( camera_Pos - wPos );
	R = reflect(-L, N);//normalize(2*dot(N, L)*N-L);
	H = normalize((L+V)/2);

	NDL = dot(N,L);
    NDV = dot(N,V);
    NDH = dot(N,H);
    VDR = dot(V,R);
	VDH = dot(V,H);


	vec3 Li = light_Int*light_Color/dot(toLight,toLight);
	vec3 ambient = mat_AmbientColor*ambOcc*baseColor; // Todo: Occ의 의미를 왜 반대로 쓰지?

	vec3 outColor = emission + brdf() * Li * max(0,NDL) + ambient;


	//debug
	//outColor = vec3((map_Flags & MF_Kd));
	//outColor = albedo.rgb;
	//outColor = vec3(Roughness);
	// outColor = vec3(ambOcc, roughness, metalness);
	// outColor = F0;
	// outColor = fresnelSchlick();
	// outColor = vec3(NDV);
	// outColor = vec3(distributionGGX());
	// outColor = ambient;

	convertLinearToSRGB(outColor);
	FragColor = vec4(outColor, 1.0-mat_Opacity);
}