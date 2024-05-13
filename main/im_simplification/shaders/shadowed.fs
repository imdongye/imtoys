#version 410 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;
in vec4 shadowFragPos;

/* light */
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
	float ZNear;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;

/* matarial */
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
/* texture */

/* etc */
uniform vec3 cam_Pos;
uniform float gamma = 2.2;

// 포아송 분포
const vec2 poissonDisk[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);
// psuedo random
float random(int i)
{
    vec4 seed4 = gl_FragCoord.xyzx;
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(float(i+123)*dot_product) * 43758.5453);
}
float shadowing()
{
	if( !shadow.Enabled ) return 1.f;

	float shadowFactor = 1.0;
	vec3 shadowClipPos = shadowFragPos.xyz/shadowFragPos.w;
	// nc(clip space)-1~1 => texpos 0~1
	vec3 shadowTexPos = (shadowClipPos+1)*0.5f;
	const float bias = 0.01;
	float depth = shadowTexPos.z ;
    float lightDepth = 1.0;//texture(map_Shadow, shadowTexPos.xy).r;
    
    // Percentage Closer Filtering PCF
    // shadow map에서 오쿨루더 사이의 거리도 알수있다.
    const float hardness = 10.0;
    const int nrDisk = 4;
    const float factorAddition = 1.0/float(nrDisk);
    
    for( int i=0; i<nrDisk; i++ ) {
        float theta = random(0)*3.141592*2;
        mat2 shadowRot = mat2(cos(theta), sin(theta), -sin(theta), cos(theta));
        vec2 shiftPos = poissonDisk[i]/hardness;
        shiftPos *= shadowRot;
        shiftPos += shadowTexPos.xy;
        lightDepth = texture( map_Shadow, shiftPos ).z;
        if ( lightDepth+bias >  depth ) {
            shadowFactor += factorAddition;
        }
    }

	// 1이아니라 map_Shadow 최대값이어야함
	// todo : texture최대값찾기
	if( shadowTexPos.z>=1 ) return 1.f;

	return shadowFactor;
}

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 L = normalize(lit.Pos-wPos);
	vec3 V = normalize(cam_Pos - wPos);
	vec3 R = 2*dot(N,L)*N-L;

	float visibility = shadowing();

	vec4 albelo = vec4(mat.BaseColor,1);

	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lit.Intensity*lambertian*albelo.rgb;
	vec3 ambient = 0.1f*albelo.rgb;
	vec3 specular = pow(max(0,dot(R,V)), mat.Shininess) * lambertian * vec3(1);
	vec3 outColor = diffuse+ambient+specular;
	outColor *= visibility;

    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
