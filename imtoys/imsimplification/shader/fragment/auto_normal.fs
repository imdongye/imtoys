//
// bump map, 노멀맵 : per pixel per vert와의 차이 생각해보기
//		non - linear
//		texel 안에서 리니어함
// bump height, tex delta 관계
//
// 노멀맵에서 범프맵을 만들때 적분을 못해서 미분값을 가지고 포아송 방정식(자코비, 가우스세이델)을 풀면된다.
// 

#version 410 core
out vec4 FragColor;

const float PI = 3.1415926;

in vec3 wPos;
in vec3 wNor;
in vec2 tUv;

/* light */
uniform int shadowEnabled = -1;
uniform vec3 lightDir = vec3(1,0,0);
uniform vec3 lightColor = vec3(1);
uniform float lightInt = 0.8;

/* matarial */
uniform float ambInt = 0.1;
uniform int shininess = 20;
uniform vec3 Kd = vec3(1,1,0);
/* texture */
uniform int texCount = 0;
uniform float texGamma = 1; // suppose linear space
uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;
/* etc */
uniform vec3 cameraPos;
uniform float gamma = 2.2; 
uniform float texDelta = 0.00001;
uniform float bumpHeight = 100;

mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(tUv), st2 = dFdy(tUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
	vec3 N;
	N = normalize(wNor);
	if( texCount>0 )
	{
		vec3 tNor, leftUp;
		mat3 TBN = getTBN( N );
		leftUp = texelFetch(map_Bump0, ivec2(0), 0).xyz;
		/* bump map */
		if(leftUp.y==leftUp.z) {
			float Bu = texture(map_Bump0, tUv+vec2(texDelta,0)).r
						- texture(map_Bump0, tUv+vec2(-texDelta,0)).r;
			float Bv = texture(map_Bump0, tUv+vec2(0,texDelta)).r
						- texture(map_Bump0, tUv+vec2(0,-texDelta)).r;
			tNor = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1);
			N = normalize(TBN*tNor);
		}
		/* normal map */
		else {
			tNor = texture(map_Bump0, tUv).xyz;
			tNor = tNor*2-vec3(1);
			N = normalize(TBN*tNor);
		}
		N = normalize(TBN*tNor);
	}
	vec3 L = normalize(lightDir);
	vec3 V = normalize(cameraPos - wPos);
	vec3 R = 2*dot(N,L)*N-L;

	float visibility = 1.0;
	if( shadowEnabled > 0 )
	{
		// ...
	}

	vec4 albelo = (texCount>1) ? pow(texture(map_Kd0, tUv),vec4(texGamma)) : vec4(Kd,1);

	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lightInt*lambertian*albelo.rgb;
	vec3 ambient = ambInt*albelo.rgb;
	vec3 specular = pow(max(0,dot(R,V)), shininess) * lambertian * vec3(1);
	vec3 outColor = diffuse+ambient+specular;
	outColor *= visibility;

    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
