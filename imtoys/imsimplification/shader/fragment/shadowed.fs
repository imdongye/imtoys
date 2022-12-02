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
uniform sampler2D shadowMap;
/* matarial */
uniform float ambInt = 0.1;
uniform int shininess = 20;
uniform vec3 Kd = vec3(1,1,0);
/* texture */

/* etc */
uniform vec3 cameraPos;
uniform float gamma = 2.2;

float shadowing()
{
	if( shadowEnabled < 0 ) return 1;

	float shadowFactor = 1.0;
	vec3 shadowClipPos = shadowFragPos.xyz/shadowFragPos.w;
	// nc(clip space)-1~1 => texpos 0~1
	vec3 shadowTexPos = (shadowClipPos+1)*0.5f;
	const float bias = 0.01;
	float d = shadowTexPos.z ;
	float td = texture(shadowMap, shadowTexPos.xy).r;

	// 1이아니라 shadowMap의 최대값이어야함
	// todo : texture최대값찾기
	if( shadowTexPos.z>=1 && td>=1 ) return 1;

	if(td < d-bias) shadowFactor = 0.1;

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
