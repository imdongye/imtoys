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

uniform bool use_IBL;
uniform sampler2D map_Light;


uniform vec3 light_Pos;
uniform vec3 light_Color;
uniform float light_Int;
uniform vec3 camera_Pos;

uniform int idx_Brdf = 0;
uniform int idx_D = 0;
uniform int idx_G = 0;
uniform int idx_F = 0;
uniform int idx_LitMod = 0;
uniform int nr_ibl_w_samples = 50;


// gamma correction
void convertLinearToSRGB( inout vec3 rgb ){
    rgb.r = rgb.r<0.0031308?(12.92*rgb.r):(1.055*pow(rgb.r,0.41667)-0.055);
    rgb.g = rgb.g<0.0031308?(12.92*rgb.g):(1.055*pow(rgb.g,0.41667)-0.055);
    rgb.b = rgb.b<0.0031308?(12.92*rgb.b):(1.055*pow(rgb.b,0.41667)-0.055);
}
mat3 getTBN0(vec3 N) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t-st2.s*st1.t;
	return mat3(normalize((Q1*st2.t-Q2*st1.t)*D),
				normalize((Q2*st1.s-Q1*st2.s)*D), N);
}
// https://gamedev.stackexchange.com/questions/86530/is-it-possible-to-calculate-the-tbn-matrix-in-the-fragment-shader
// Todo: TB의 Precompute, Geometry(victor), Fragment 성능 비교
mat3 getTBN(vec3 N) {
	vec3 dp1 = dFdx( wPos );
    vec3 dp2 = dFdy( wPos );
    vec2 duv1 = dFdx( mUv );
    vec2 duv2 = dFdy( mUv );

    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}
vec2 rand(vec2 st, int i) {
    return vec2( 
		fract(sin(dot(st+i*4.1233, vec2(12.9898, 78.2336))) * 43758.5453),
    	fract(sin(dot(st+i*6.8233, vec2(39.8793, 83.2402))) * 22209.2896)
	);
}

//*****************************************
//            Rendering Equation
//*****************************************
vec3 N, L, V, H, Rl, Rv;
float NDL, NDV, NDH, VDRl, VDH;
vec3 baseColor, F0, emission;
float roughness, metalness, ambOcc;


vec3 OrenNayar() {
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
	vec3 l2 = 0.17*baseColor*cosTi*(aa/(aa+0.13))*(1-cosPhi*pow(2*beta/PI,2));
	return vec3(l1)+l2;
}

vec3 brdfLambertian() { // Diffuse
	return vec3(1/PI);
}
vec3 brdfPhong() { // Specular
	float normalizeFactor = (mat_Shininess+1)/(2*PI);
	return mat_AmbientColor + vec3(1) * normalizeFactor * pow( max(0,VDRl), mat_Shininess );
}
vec3 brdfBlinnPhong() { // Specular
	// vec3 H = (V+L)/2; // 성능을위해 노멀라이즈 하지 않음.
	// float NDH = max(0,dot(N, H));
	float normalizeFactor = (mat_Shininess+1)/(2*PI);
	return mat_AmbientColor + vec3(1) * normalizeFactor * pow( max(0,NDH), mat_Shininess );
}



float distributionBlinnPhong() {
	float a = roughness*roughness;
	float k = 2/(a*a)-2; // instead of Ns(Shininess)
	float normalizeFactor = 1/(PI*a*a); // for integral to 1
	return normalizeFactor * pow( max(0,NDH), k );
}

float distributionGG1() { // Todo: NDH 큰부분에서 음수 왜그런지 모르겠음
	float a = roughness*roughness;
	float aa = a*a;
	float theta = acos(NDH);
	float denom = PI * pow(NDH,4) * pow(aa+pow(tan(theta),2),2);
	return aa/denom;
}
// http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
float distributionGGX() {
	float a = roughness*roughness;
	float aa = a*a;
	float d = (NDH*aa - NDH)*NDH + 1.0;
	return aa/(d*d*PI);
}
float distributionGGX2() {
	float a = roughness*roughness;
	float aa = a*a;
	float mNDH = max(0, NDH);
	float NDH2 = mNDH*mNDH;
	float num = aa;
	float denom = NDH2*(aa-1)+1;
	denom = PI*denom*denom;
	return num/denom;
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

float _geometryShlickGGX(float num) {
	float r = roughness+1;
	float k = r*r/8.0;
	float denom = num*(1-k)+k;
	return num/denom;
}
float geometrySmith() {
	float ggx2 = _geometryShlickGGX(max(NDV, 0));
	float ggx1 = _geometryShlickGGX(max(NDL, 0));
	return ggx1*ggx2;
}

vec3 fresnelSchlick(float cosTheta) {
	float ratio = max(0,pow(1-cosTheta, 5));
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
	case 1: G = geometrySmith(); break;
	}
	switch(idx_F) {
	case 0: F = fresnelSchlick(NDV); break;
	case 1: F = fresnelSchlick(NDH); break;
	case 2: F = fresnelSchlick(VDH); break;
	}
	// return D*G*F / (4*max(NDL,0)*max(NDV,0)); // 내적 max하면 음수??
	return D*G*F / (4*NDL*NDV);
}
vec3 brdfPoint() {
	vec3 diffuse = baseColor * mix(brdfLambertian(), vec3(0), metalness);

	vec3 specular = vec3(0);
	switch(idx_Brdf) {
	case 0: specular = brdfPhong(); break;
	case 1: specular = brdfBlinnPhong(); break;
	case 2: specular = brdfCookTorrance(); break;
	case 3: specular = OrenNayar(); break;
	}
	return diffuse+specular;
}
vec3 pointLighting() {
	vec3 toLight = light_Pos-wPos;
	L = normalize( toLight );
	Rl = reflect(-L, N);//normalize(2*dot(N, L)*N-L);
	H = normalize(L+V);

	NDL = dot(N,L);
    NDH = dot(N,H);
    VDRl = dot(V,Rl);
	VDH = dot(V,H);

	vec3 Li = light_Int*light_Color/dot(toLight,toLight); // radiance
	vec3 ambient = mat_AmbientColor*ambOcc*baseColor;

	return emission + brdfPoint() * Li * max(0,NDL) + ambient;
}

/* spherical coordinate convertion */
// theta of right vector is 0
// phi of up vector is 0 
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 uvFromDir(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv.y += 0.5;
    return uv;
}
vec3 dirFromUv(vec2 uv) {
	float theta = 2*PI*uv.x;
	float phi = PI*(uv.y-0.5);
	return vec3( cos(theta)*cos(phi), sin(phi), sin(theta)*cos(phi) );
}
vec3 ibSamplingLighting() {
	vec3 ambient = mat_AmbientColor*ambOcc*baseColor;
	vec3 sum = vec3(0);

	for(int i=0; i<nr_ibl_w_samples; i++) for(int j=0; j<nr_ibl_w_samples; j++) {
		vec2 uv = vec2( i/float(nr_ibl_w_samples), j/float(nr_ibl_w_samples) ); // Todo 레귤러렌덤

		L = dirFromUv(uv);
		Rl = reflect(-L, N);//normalize(2*dot(N, L)*N-L);
		H = normalize(L+V);

		NDL = dot(N,L);
		NDH = dot(N,H);
		VDRl = dot(V,Rl);
		VDH = dot(V,H);

		if( NDL<0 ) // out of hemisphere
			continue;

		float phi = (uv.y-0.5)*PI; // Todo: rm dup
		vec3 Li = light_Int * texture(map_Light, uv).rgb;
		
		sum += brdfPoint()*Li*NDL*cos(phi);
	}
	float dist2 = 10; // radiance
	vec3 integ = sum/(nr_ibl_w_samples*nr_ibl_w_samples * dist2);
	return emission + integ + ambient;
}


// 노멀기준에서 roughness에 따라 샘플링 분포 설정(GGX)
vec3 importanceSampleGGX(vec2 uv) {
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
// Rodrigues ver https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
vec3 importanceSampleGGX2(vec2 uv) {
	float a = roughness*roughness;
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

vec3 ibImportanceSamplingLighting() {
	vec3 ambient = mat_AmbientColor*ambOcc*baseColor;
	vec3 sum = vec3(0);

	for(int i=0; i<nr_ibl_w_samples; i++) for(int j=0; j<nr_ibl_w_samples; j++) {
		vec2 uv = vec2( i/float(nr_ibl_w_samples), j/float(nr_ibl_w_samples) );
		uv = rand(uv, i);

		H = importanceSampleGGX2(uv);
		L = reflect(-V, H);
		Rl = reflect(-L, N);

		NDL = dot(N,L);
		NDH = dot(N,H);
		VDRl = dot(V,Rl);
		VDH = dot(V,H);

		if( NDL<0 ) // out of hemisphere
			continue;

		float phi = (uv.y-0.5)*PI; // Todo: rm dup
		vec3 Li = light_Int * texture(map_Light, uvFromDir(L)).rgb;

		// brdfPoint에서 distributionGGX와 분모, cos(phi)와 brdf의 NDH 없애서 최적화 가능
		// brdf 여러개 써보기 위해서 일단 ggx ndf로 나눈다. 
		sum += brdfPoint()*Li*NDL*cos(phi) / distributionGGX(); // Todo: 분모 p가 제대로 적용 안돼서 아티펙트 보이는것 같음.

		// vec3 spec = Li/(4*NDV);
		// sum += spec;
		// sum += Li;
	}
	float dist2 = 10; // radiance
	vec3 integ = sum/(nr_ibl_w_samples*nr_ibl_w_samples * dist2);
	return emission + integ + ambient;
}

void main() {
	vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
	if( dot(N,faceN)<0 ) N = -N; // 모델의 내부에서 back face일때 노멀을 뒤집는다.
	float alpha = 1.0;
	N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);

	baseColor = mat_BaseColor;
	if( (map_Flags & MF_BASE_COLOR)>0 ) {
		vec4 rgba = texture( map_BaseColor, mUv );
		baseColor = rgba.rgb;
		alpha = rgba.a;
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
	// emission = mat_EmissionColor;
	emission = vec3(0);
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
	V = normalize( camera_Pos - wPos );
    NDV = dot(N,V);
	Rv = reflect(-V, N);

	vec3 outColor;
	switch(idx_LitMod) {
	case 0: outColor = pointLighting(); break;
	case 1: outColor = ibSamplingLighting(); break;
	case 2: outColor = ibImportanceSamplingLighting(); break;
	case 3: outColor = ibSamplingLighting(); break;
	}

	//debug
	//outColor = vec3((map_Flags & MF_Kd));
	//outColor = albedo.rgb;
	// outColor = vec3(ambOcc, roughness, metalness);
	// outColor = F0;
	// outColor = fresnelSchlick();
	// outColor = vec3(NDV);
	// outColor = vec3(distributionGGX());
	// outColor = ambient;
	//outColor = vec3(1)*geometrySmith();
	// outColor = vec3(geometrySmith());
	// outColor = fresnelSchlick();
	// outColor = vec3(NDH);
	// outColor = fresnelSchlick(HDV);
	// outColor = vec3(NDV);
	// outColor = emission;
	// outColor = vec3(roughness);
	// outColor = texture(map_Light, uvFromDir(Rv)).rgb;
	// outColor = texture(map_Light, uvFromDir(dirFromUv(uvFromDir(Rv)))).rgb;
	// outColor = Rv*0.5+0.5;

	convertLinearToSRGB(outColor);
	FragColor = vec4(outColor, alpha);
}