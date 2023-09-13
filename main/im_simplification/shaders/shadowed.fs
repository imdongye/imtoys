#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 tUv;
in vec4 shadowFragPos;

/* light */
uniform int shadowEnabled = -1;
uniform vec3 lightDir = vec3(1,0,0);
uniform vec3 lightColor = vec3(1);
uniform float lightInt = 0.8;
uniform sampler2D map_Shadow;
/* matarial */
uniform float ambInt = 0.1;
uniform int shininess = 20;
uniform vec3 Kd = vec3(1,1,0);
/* texture */

/* etc */
uniform vec3 cameraPos;
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
	if( shadowEnabled < 0 ) return 1;

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
	if( shadowTexPos.z>=1 ) return 1;

	return shadowFactor;
}

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 L = normalize(lightDir);
	vec3 V = normalize(cameraPos - wPos);
	vec3 R = 2*dot(N,L)*N-L;

	float visibility = shadowing();

	vec4 albelo = vec4(Kd,1);

	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lightInt*lambertian*albelo.rgb;
	vec3 ambient = ambInt*albelo.rgb;
	vec3 specular = pow(max(0,dot(R,V)), shininess) * lambertian * vec3(1);
	vec3 outColor = diffuse+ambient+specular;
	outColor *= visibility;

    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
