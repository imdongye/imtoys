#version 410 core
layout(location=0) out vec4 FragColor;
const float PI = 3.1415926535;

in vec3 wPos;
in vec3 wNor;
in vec4 lPos;
in vec4 oColor;


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
	float ZNear;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;


// gamma correction
void convertLinearToSRGB( inout vec3 rgb ){
    rgb.r = rgb.r<0.0031308?(12.92*rgb.r):(1.055*pow(rgb.r,0.41667)-0.055);
    rgb.g = rgb.g<0.0031308?(12.92*rgb.g):(1.055*pow(rgb.g,0.41667)-0.055);
    rgb.b = rgb.b<0.0031308?(12.92*rgb.b):(1.055*pow(rgb.b,0.41667)-0.055);
}


// iqint3 From: https://www.shadertoy.com/view/4tXyWN
float iqint3( uvec2 x ) {
    uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return float(n) * (1.0/float(0xffffffffU));
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

// projection
float zTexToZView(float zTex) { // [0,1] -> [near, far]
	// [-1,1](non linear) -> [near, far](linear)
	//return -2.0*light_z_Far*light_z_Near / ( zNdc*(light_z_Far-light_z_Near) + light_z_Far+light_z_Near );
	// [0,1] -> [near, far]
	return shadow.ZFar*shadow.ZNear / ( (shadow.ZNear-shadow.ZFar)*zTex-shadow.ZNear ); // 왜 안되지
}
// float zClipToZView(float zClip) { // [-near, far] -> [near, far]
// 	return ((light_z_Far-light_z_Near)*zClip + 2.0*light_z_Near*light_z_Far) / (light_z_Far+light_z_Near);
// }

float PCF(vec2 shadowTexPos, float realDepth, vec2 sampleRadiusUv) // 0.002
{
	float visibility = 0.0;
	float startTheta = iqint3(uvec2(gl_FragCoord.xy)+uvec2(84,137))*PI;
	realDepth = min(realDepth, 1.0); // for shadowmap border color 1


	// From: https://www.shadertoy.com/view/4l3yRM
	// Todo for O(1): https://www.shadertoy.com/view/XtlXDM
	for( int i = 0; i < NR_PCF_SAMPLE; i++ ){
        vec2 samplePos = shadowTexPos
			+ vogelSample(i, NR_PCF_SAMPLE, startTheta) * sampleRadiusUv;
        float lightDepth = texture(map_Shadow, samplePos).r;

		if( lightDepth+SHADOW_BIAS > realDepth ) {
			visibility += 1.0;
		}
    }
	visibility /= float(NR_PCF_SAMPLE);

	return visibility;
}

// ortho directional light
float shadowing()
{
	if(!shadow.Enabled)
		return 1.f;

	const float bias = 0.001;
	vec3 shadowNDC_pos = lPos.xyz/lPos.w;
	vec3 shadowTexCoord = (shadowNDC_pos)*0.5+vec3(0.5);
	float realDepth = shadowTexCoord.z;

	// return shadowTexCoord.z;
	// return texture(map_Shadow, shadowTexCoord.xy).r;
	return PCF(shadowTexCoord.xy, realDepth, shadow.RadiusUv);
	// return shadowTexCoord.z;
	// return (realDepth-shadow_z_Near)/(shadow_z_Far-shadow_z_Near);

	// STEP 1: blocker search
	float avgBlockerDepth, nrBlockers;
	// Using similar triangles from the surface point to the area light
	// (샘플링 콘의 비율을 일정하게 하기위함.)
	vec2 sRadiusTUv = shadow.RadiusUv*shadow.TexelSize;
	vec2 searchRegionRadiusUV = sRadiusTUv * (realDepth - shadow.ZNear) / realDepth;
	findBlocker( avgBlockerDepth, nrBlockers, shadowTexCoord, searchRegionRadiusUV);
	// return nrBlockers/float(NR_FIND_BLOCKER_SAMPLE);
	if( nrBlockers == 0 ) return 1.0;
	if( nrBlockers == NR_FIND_BLOCKER_SAMPLE ) return 0.0;


	// STEP 2: penumbra size
	float avgBlockerDepthWorld = zTexToZView(avgBlockerDepth); // linearize
	// Using similar triangles between the area light, the blocking plane and the surface point
	vec2 wPenumbraUv = (realDepth - avgBlockerDepthWorld)*sRadiusTUv / avgBlockerDepthWorld;
	// Project UV size to the near plane of the light
	vec2 filterRadiusUv = wPenumbraUv*shadow.ZNear/realDepth;


	// STEP 3: filtering
	return PCF(shadowTexCoord.xy, shadowTexCoord.z, filterRadiusUv);
}




void main()
{    
	vec3 faceN = normalize( cross( dFdx(wPos), dFdy(wPos) ) );
    vec3 L = normalize(lit.Pos - wPos);
	vec3 N = normalize(wNor);
	// if( dot(N,faceN)<0 )
	// 	N = -N;
	float NDL = max(dot(N, L),0);
    vec3 outColor;
	outColor = (0.05f+0.95f*shadowing()) * NDL * oColor.xyz;


	// debug
	// outColor = vec3(shadowing());
	// outColor = N*0.5+vec3(0.5);

	convertLinearToSRGB(outColor);
    FragColor = vec4(outColor, 1);
}
