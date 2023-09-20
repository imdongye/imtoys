//
// todo: 노멀맵에서 범프맵을 만들때 적분을 못해서 미분값을 가지고 포아송 방정식(자코비, 가우스세이델)을 풀면된다.
#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

/* global const */
const float gamma = 2.2f;

/* camera */
uniform vec3 cameraPos;
uniform mat4 viewPat;
uniform mat4 projMat;

/* light */
uniform vec3 lightPos = vec3(1,1,0);
uniform vec3 lightColor = vec3(1);
uniform float lightInt = 0.8;
uniform int shadowEnabled = 0;
uniform mat4 shadowVP = mat4(1);
uniform sampler2D map_Shadow;

/* matarial */
uniform vec4 Kd;
uniform vec4 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform vec3 Tf;
uniform float Ni;
uniform int map_Flags;
uniform sampler2D map_Kd;
uniform sampler2D map_Ks;
uniform sampler2D map_Ka;
uniform sampler2D map_Ns;
uniform sampler2D map_Bump;
uniform float texDelta = 0.00001;
uniform float bumpHeight = 100;

mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
	vec3 N = normalize(wNor);

	if( (map_Flags&(3<<4)) > 0 ) // has bump
	{
		mat3 TBN = getTBN( N );
		vec3 tNor; // tangent normal
		/* bump map */
		if( (map_Flags&(1<<4)) > 0 ) {
			float Bu = texture(map_Bump, mUv+vec2(texDelta,0)).r
						- texture(map_Bump, mUv+vec2(-texDelta,0)).r;
			float Bv = texture(map_Bump, mUv+vec2(0,texDelta)).r
						- texture(map_Bump, mUv+vec2(0,-texDelta)).r;
			tNor = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1);
		}
		/* normal map */
		else {
			tNor = texture(map_Bump, mUv).xyz*2-vec3(1);
		}
		N = normalize(TBN*tNor);
	}
	vec3 L = normalize(lightPos - wPos);
	vec3 V = normalize(cameraPos - wPos);
	vec3 R = 2*dot(N,L)*N-L;

	if( shadowEnabled > 0 )
	{
		// ...
	}

	vec4 albelo = ( (map_Flags&1) > 0 ) ? texture(map_Kd, mUv) : Kd;
	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lightInt*lambertian*albelo.rgb;
	vec3 ambient = Ka;
	vec3 specular = pow(max(0,dot(R,V)), Ks.a) * lambertian * vec3(1);
	vec3 outColor = diffuse+ambient+specular;

    outColor = pow(outColor, vec3(1/gamma));
	outColor = texture(map_Kd, mUv).rgb;
    fragColor = vec4(outColor, 1);

	fragColor = albelo;
}
