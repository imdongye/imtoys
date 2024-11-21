/*

2023fall advenced rendering theorem 2 

version rm sampings, and only brdfCookTorrance

*/
#version 410
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;
in vec4 lPos;

const float PI = 3.1415926535;



// material
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
struct Material {
	vec3 BaseColor;
	vec3 SpecColor;
	vec3 AmbientColor;
	vec3 EmissionColor;
	vec3 F0;
	float Transmission;
	float Refraciti;
	float Opacity;
	float Shininess;
	float Roughness;
	float Metalness;
	float BumpHeight;
	float TexDelta;
};
uniform Material mat;
uniform int map_Flags;
uniform sampler2D map_BaseColor;
uniform sampler2D map_Specular;
uniform sampler2D map_Bump;
uniform sampler2D map_AmbOcc;
uniform sampler2D map_Roughness;
uniform sampler2D map_Metalness;
uniform sampler2D map_Emission;
uniform sampler2D map_Opacity;


// camera
uniform vec3 cam_Pos;

//light
uniform sampler2D map_Light;
uniform sampler2D map_Irradiance;
uniform sampler3D map_PreFilteredEnv;
uniform sampler2D map_PreFilteredBRDF;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;

struct ShadowDirectional {
	bool Enabled;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;



// for testing
uniform int idx_LitMod = 1;









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

const int nrDisk = 4;
const vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);
float rand(int i) {
    vec4 seed4 = gl_FragCoord.zxyx;
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(float(i+123)*dot_product) * 42758.5453);
}

// iqint3 From: https://www.shadertoy.com/view/4tXyWN
float iqint3( uvec2 x ) {
    uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return float(n) * (1.0/float(0xffffffffU));
}
float randWith2(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}








// shadowing magic number
const int NR_FIND_BLOCKER_SAMPLE = 32;
const int NR_PCF_SAMPLE = 32;
const float SHADOW_BIAS = 0.001; // Todo: non linear한 depth값에 적용하는거라 정필요할듯

vec2 vogelSample( int i, int nrSample, float startTheta ) {
	const float GoldenAngle = 2.4;
	float idx = float(i);
	float r = sqrt(idx+0.5)/sqrt(float(nrSample));
	float theta = idx*GoldenAngle+startTheta;
	return vec2( cos(theta), sin( theta ) )*r;
}
// Returns average blocker depth in the search region, as well as the number of found blockers.
// Blockers are defined as shadow-map samples between the surface point and the light.
// nrBlocker for early out.
void findBlocker( out float avgBlockerDepth, out float nrBlockers, vec3 shadowTexCoord, vec2 searchRegionRadiusUV ) {
	float curDepth = shadowTexCoord.z;
	float blockerSum = 0;
	nrBlockers = 0;
	float startTheta = iqint3(uvec2(gl_FragCoord.xy))*PI;

	for( int i = 0; i < NR_FIND_BLOCKER_SAMPLE; i++ ) {
        vec2 samplePos = shadowTexCoord.xy 
			+ vogelSample(i, NR_FIND_BLOCKER_SAMPLE, startTheta) * searchRegionRadiusUV;
        float minDepth = texture(map_Shadow, samplePos).r;

		if( minDepth+SHADOW_BIAS < curDepth ) {
			blockerSum += minDepth;
            nrBlockers++;
		}
    }

	avgBlockerDepth = blockerSum / float(nrBlockers);
}

float zTexToZView(float zTex) { // [0,1] -> [near, far]
	// [-1,1](non linear) -> [near, far](linear)
	//return -2.0*light_z_Far*light_z_Near / ( zNdc*(light_z_Far-light_z_Near) + light_z_Far+light_z_Near );
	// [0,1] -> [near, far]
	return shadow.ZFar*shadow.ZNear / ( (shadow.ZNear-shadow.ZFar)*zTex-shadow.ZNear ); // 왜 안되지
}
// float zClipToZView(float zClip) { // [-near, far] -> [near, far]
// 	return ((light_z_Far-light_z_Near)*zClip + 2.0*light_z_Near*light_z_Far) / (light_z_Far+light_z_Near);
// }

float PCF(vec2 shadowTexPos, float curDepth, vec2 sampleRadiusUv) // 0.002
{
	float visibility = 0.0;
	float startTheta = iqint3(uvec2(gl_FragCoord.xy)+uvec2(84,137))*PI;

	// From: https://www.shadertoy.com/view/4l3yRM
	// Todo for O(1): https://www.shadertoy.com/view/XtlXDM
	for( int i = 0; i < NR_PCF_SAMPLE; i++ ){
        vec2 samplePos = shadowTexPos
			+ vogelSample(i, NR_PCF_SAMPLE, startTheta) * sampleRadiusUv;
        float minDepth = texture(map_Shadow, samplePos).r;

		if( minDepth+SHADOW_BIAS > curDepth ) {
			visibility += 1.0;
		}
    }
	visibility /= float(NR_PCF_SAMPLE);

	return visibility;
}

float shadowing()
{
	if(!shadow.Enabled)
		return 1.f;

	const float bias = 0.001;
	vec3 shadowNDC_pos = lPos.xyz/lPos.w;
	vec3 shadowTexCoord = (shadowNDC_pos+1)*0.5;
	float lDepth = zTexToZView(shadowTexCoord.z); // [0, 1] -> [n, f]

	// return shadowTexCoord.z;
	// return texture(map_Shadow, shadowTexCoord.xy).r;
	return PCF(shadowTexCoord.xy, shadowTexCoord.z, shadow.RadiusUv);
	// return shadowTexCoord.z;
	// return (lDepth-shadow_z_Near)/(shadow_z_Far-shadow_z_Near);

	// STEP 1: blocker search
	float avgBlockerDepth, nrBlockers;
	// Using similar triangles from the surface point to the area light
	// (샘플링 콘의 비율을 일정하게 하기위함.)
	vec2 sRadiusTUv = shadow.RadiusUv*shadow.TexelSize;
	vec2 searchRegionRadiusUV = sRadiusTUv * (lDepth - shadow.ZNear) / lDepth;
	findBlocker( avgBlockerDepth, nrBlockers, shadowTexCoord, searchRegionRadiusUV);
	// return nrBlockers/float(NR_FIND_BLOCKER_SAMPLE);
	if( nrBlockers == 0 ) return 1.0;
	if( nrBlockers == NR_FIND_BLOCKER_SAMPLE ) return 0.0;


	// STEP 2: penumbra size
	float avgBlockerDepthWorld = zTexToZView(avgBlockerDepth); // linearize
	// Using similar triangles between the area light, the blocking plane and the surface point
	vec2 wPenumbraUv = (lDepth - avgBlockerDepthWorld)*sRadiusTUv / avgBlockerDepthWorld;
	// Project UV size to the near plane of the light
	vec2 filterRadiusUv = wPenumbraUv*shadow.ZNear/lDepth;


	// STEP 3: filtering
	return PCF(shadowTexCoord.xy, shadowTexCoord.z, filterRadiusUv);
}







//*****************************************
//            Rendering Equation
//*****************************************
vec3 N, L, V, H, Rl, Rv;
float NDL, NDV, NDH, VDRl, VDH;
vec3 base_color, F0, emission;
float roughness, metalness, amb_occ;


// http://cwyman.org/code/dxrTutors/tutors/Tutor14/tutorial14.md.html
float distributionGGX() {
	float a = roughness*roughness;
	float aa = a*a;
	float d = (NDH*aa - NDH)*NDH + 1.0;
	return aa/(d*d*PI);
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
vec3 brdfLambertian() { // Diffuse
	return vec3(1/PI);
}
vec3 brdfCookTorrance() {
	float D, G;
	vec3 F;
	D = distributionGGX();
	G = geometrySmith();
	F = fresnelSchlick(VDH); // ndv, ndh
	// return D*G*F / (4*max(NDL,0)*max(NDV,0)); // 내적 max하면 음수??
	return D*G*F / (4*NDL*NDV); // or 4PI
}
vec3 brdfPoint() {
	vec3 diffuse = base_color * mix(brdfLambertian(), vec3(0), metalness);

	vec3 specular = brdfCookTorrance();
	return diffuse+specular;
}
vec3 pointLighting() {
	vec3 toLight = lit.Pos-wPos;
	L = normalize( toLight );
	Rl = reflect(-L, N);//normalize(2*dot(N, L)*N-L);
	H = normalize(L+V);

	NDL = dot(N,L);
    NDH = dot(N,H);
    VDRl = dot(V,Rl);
	VDH = dot(V,H);

	vec3 Li = lit.Intensity*lit.Color/dot(toLight,toLight); // radiance
	vec3 ambient = mat.AmbientColor*amb_occ*base_color;
	// return vec3(shadowing());
	return emission + shadowing() * brdfPoint() * Li * max(0,NDL) + ambient;
}

/* spherical coordinate convertion */
// theta of right vector is 0, use clock wise!
// phi of up vector is 0, downvector is PI
// 최적화 필요
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


vec3 ibPrefilteredLighting() {
	// vec2 uvIrr = uvFromDir(N);
	vec2 uvIrr = uvFromDir(N);
	vec2 uvPfenv = uvFromDir(Rv);
	// return  baseColor * texture(map_Irradiance, uvIrr).rgb;
	// return texture(map_PreFilteredEnv, uvPfenv).rgb;
	// return texture(map_PreFilteredEnv, vec3(uvPfenv,roughness)).rgb;
	vec3 diff = mix(base_color,vec3(0), metalness) * texture(map_Irradiance, uvIrr).rgb  * lit.Intensity*0.01;
	vec3 spec = texture(map_PreFilteredEnv, vec3(uvPfenv,roughness)).rgb * lit.Intensity*0.01;
	vec2 brdf = texture(map_PreFilteredBRDF, vec2(NDV, roughness)).rg;
	spec = spec*(F0*brdf.x + brdf.y);
	return diff + spec;
}








void main() {
	vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
	if( dot(N,faceN)<0 ) N = -N; // 모델의 내부에서 back face일때 노멀을 뒤집는다.
	float alpha = 1.0;
	N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);

	base_color = mat.BaseColor;
	if( (map_Flags & MF_BASE_COLOR)>0 ) {
		vec4 rgba = texture( map_BaseColor, mUv );
		base_color = rgba.rgb;
		alpha = rgba.a;
	}
	amb_occ = 1.f;
	if( (map_Flags & MF_AMB_OCC)>0 ) {
		amb_occ = texture( map_AmbOcc, mUv ).r;
	}
	roughness = mat.Roughness;
	if( (map_Flags & MF_ROUGHNESS)>0 ) {
		roughness = texture( map_Roughness, mUv ).r;
	}
	metalness = mat.Metalness;
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
		amb_occ 	  = texCol.r;
		roughness = texCol.g;
		metalness = texCol.b;
	}
	if( (map_Flags & MF_NOR)>0 ) {
		mat3 tbn = getTBN(N);
		N = tbn* (texture( map_Bump, mUv ).rgb*2 - vec3(1));
	}
    F0 = mix(mat.F0, base_color, metalness);
	V = normalize( cam_Pos - wPos );
    NDV = dot(N,V);
	Rv = reflect(-V, N);

	vec3 outColor;
	switch(idx_LitMod) {
	case 0: outColor = pointLighting(); break;
	case 1: outColor = ibPrefilteredLighting(); break;
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