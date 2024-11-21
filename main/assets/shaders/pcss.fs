#version 460 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;
in vec4 lPos;

uniform vec3 cam_Pos;
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

uniform float gamma = 2.2;










// shadowing magic number
const int NR_FIND_BLOCKER_SAMPLE = 32;
const int NR_PCF_SAMPLE = 32;
const float SHADOW_BIAS = 0.001; // Todo: non linear한 depth값에 적용하는거라 정필요할듯
const float PI = 3.1415926535;

const int nr_poisson = 32;
const vec2 poisson32[] = {
	{-0.975402, -0.0711386},
	{-0.920347, -0.41142},
	{-0.883908, 0.217872},
	{-0.884518, 0.568041},
	{-0.811945, 0.90521},
	{-0.792474, -0.779962},
	{-0.614856, 0.386578},
	{-0.580859, -0.208777},
	{-0.53795, 0.716666},
	{-0.515427, 0.0899991},
	{-0.454634, -0.707938},
	{-0.420942, 0.991272},
	{-0.261147, 0.588488},
	{-0.211219, 0.114841},
	{-0.146336, -0.259194},
	{-0.139439, -0.888668},
	{0.0116886, 0.326395},
	{0.0380566, 0.625477},
	{0.0625935, -0.50853},
	{0.125584, 0.0469069},
	{0.169469, -0.997253},
	{0.320597, 0.291055},
	{0.359172, -0.633717},
	{0.435713, -0.250832},
	{0.507797, -0.916562},
	{0.545763, 0.730216},
	{0.56859, 0.11655},
	{0.743156, -0.505173},
	{0.736442, -0.189734},
	{0.843562, 0.357036},
	{0.865413, 0.763726},
	{0.872005, -0.927},
};
float shadowing01() // Soft Shadow
{
	if(!shadow.Enabled) 
		return 1.f;
	vec3 shadowClipPos = lPos.xyz/lPos.w;
	vec3 shadowTexPos = (shadowClipPos+1)*0.5f;
	float curDepth = shadowTexPos.z;

	int nr_front = 0;
	for( int i=0; i<nr_poisson; i++ ) {
		vec2 off = 0.01*poisson32[i]*shadow.RadiusUv*shadow.OrthoSize;
		float frontDepth = texture(map_Shadow, shadowTexPos.xy+off).r;
		if( curDepth < frontDepth+0.001 ) {
			nr_front++;
		}
	}
	
	return float(nr_front)/float(nr_poisson);
}


// float rand(int i) {
//     vec4 seed4 = gl_FragCoord.zxyx;
//     float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
//     return fract(sin(float(i+123)*dot_product) * 42758.5453);
// }

// // iqint3 From: https://www.shadertoy.com/view/4tXyWN
// float iqint3( uvec2 x ) {
//     uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
//     uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
//     return float(n) * (1.0/float(0xffffffffU));
// }
// float randWith2(vec2 co){
// 	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
// }


// vec2 vogelSample( int i, int nrSample, float startTheta ) {
// 	const float GoldenAngle = 2.4;
// 	float idx = float(i);
// 	float r = sqrt(idx+0.5)/sqrt(float(nrSample));
// 	float theta = idx*GoldenAngle+startTheta;
// 	return vec2( cos(theta), sin( theta ) )*r;
// }
// // Returns average blocker depth in the search region, as well as the number of found blockers.
// // Blockers are defined as shadow-map samples between the surface point and the light.
// // nrBlocker for early out.
// void findBlocker( out float avgBlockerDepth, out float nrBlockers, vec3 shadowTexCoord, vec2 searchRegionRadiusUV ) {
// 	float curDepth = shadowTexCoord.z;
// 	float blockerSum = 0;
// 	nrBlockers = 0;
// 	float startTheta = iqint3(uvec2(gl_FragCoord.xy))*PI;

// 	for( int i = 0; i < NR_FIND_BLOCKER_SAMPLE; i++ ) {
//         vec2 samplePos = shadowTexCoord.xy 
// 			+ vogelSample(i, NR_FIND_BLOCKER_SAMPLE, startTheta) * searchRegionRadiusUV;
//         float minDepth = texture(map_Shadow, samplePos).r;

// 		if( minDepth+SHADOW_BIAS < curDepth ) {
// 			blockerSum += minDepth;
//             nrBlockers++;
// 		}
//     }

// 	avgBlockerDepth = blockerSum / float(nrBlockers);
// }

// float zTexToZView(float zTex) { // [0,1] -> [near, far]
// 	// [-1,1](non linear) -> [near, far](linear)
// 	//return -2.0*light_z_Far*light_z_Near / ( zNdc*(light_z_Far-light_z_Near) + light_z_Far+light_z_Near );
// 	// [0,1] -> [near, far]
// 	return shadow.ZFar*shadow.ZNear / ( (shadow.ZNear-shadow.ZFar)*zTex-shadow.ZNear ); // 왜 안되지
// }
// // float zClipToZView(float zClip) { // [-near, far] -> [near, far]
// // 	return ((light_z_Far-light_z_Near)*zClip + 2.0*light_z_Near*light_z_Far) / (light_z_Far+light_z_Near);
// // }

// float PCF(vec2 shadowTexPos, float curDepth, vec2 sampleRadiusUv) // 0.002
// {
// 	float visibility = 0.0;
// 	float startTheta = iqint3(uvec2(gl_FragCoord.xy)+uvec2(84,137))*PI;

// 	// From: https://www.shadertoy.com/view/4l3yRM
// 	// Todo for O(1): https://www.shadertoy.com/view/XtlXDM
// 	for( int i = 0; i < NR_PCF_SAMPLE; i++ ){
//         vec2 samplePos = shadowTexPos
// 			+ vogelSample(i, NR_PCF_SAMPLE, startTheta) * sampleRadiusUv;
//         float minDepth = texture(map_Shadow, samplePos).r;

// 		if( minDepth+SHADOW_BIAS > curDepth ) {
// 			visibility += 1.0;
// 		}
//     }
// 	visibility /= float(NR_PCF_SAMPLE);

// 	return visibility;
// }

// float shadowing_pcss()
// {
// 	if(!shadow.Enabled)
// 		return 1.f;

// 	const float bias = 0.001;
// 	vec3 shadowNDC_pos = lPos.xyz/lPos.w;
// 	vec3 shadowTexCoord = (shadowNDC_pos+1)*0.5;
// 	float lDepth = zTexToZView(shadowTexCoord.z); // [0, 1] -> [n, f]

// 	// return shadowTexCoord.z;
// 	// return texture(map_Shadow, shadowTexCoord.xy).r;
// 	return PCF(shadowTexCoord.xy, shadowTexCoord.z, shadow.RadiusUv);
// 	// return shadowTexCoord.z;
// 	// return (lDepth-shadow_z_Near)/(shadow_z_Far-shadow_z_Near);

// 	// STEP 1: blocker search
// 	float avgBlockerDepth, nrBlockers;
// 	// Using similar triangles from the surface point to the area light
// 	// (샘플링 콘의 비율을 일정하게 하기위함.)
// 	vec2 sRadiusTUv = shadow.RadiusUv*shadow.TexelSize;
// 	vec2 searchRegionRadiusUV = sRadiusTUv * (lDepth - shadow.ZNear) / lDepth;
// 	findBlocker( avgBlockerDepth, nrBlockers, shadowTexCoord, searchRegionRadiusUV);
// 	// return nrBlockers/float(NR_FIND_BLOCKER_SAMPLE);
// 	if( nrBlockers == 0 ) return 1.0;
// 	if( nrBlockers == NR_FIND_BLOCKER_SAMPLE ) return 0.0;


// 	// STEP 2: penumbra size
// 	float avgBlockerDepthWorld = zTexToZView(avgBlockerDepth); // linearize
// 	// Using similar triangles between the area light, the blocking plane and the surface point
// 	vec2 wPenumbraUv = (lDepth - avgBlockerDepthWorld)*sRadiusTUv / avgBlockerDepthWorld;
// 	// Project UV size to the near plane of the light
// 	vec2 filterRadiusUv = wPenumbraUv*shadow.ZNear/lDepth;


// 	// STEP 3: filtering
// 	return PCF(shadowTexCoord.xy, shadowTexCoord.z, filterRadiusUv);
// }



void main()
{    
    vec3 L = normalize(lit.Pos - wPos);
    // vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	vec3 N = normalize(wNor);
	float shading = shadowing01()*max(dot(N, L),0);
    vec3 outColor = vec3(shading);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
