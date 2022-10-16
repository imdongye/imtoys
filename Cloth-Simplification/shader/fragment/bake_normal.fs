
#version 410 core
out vec4 FragColor;

const float PI = 3.1415926;
const float TEX_DELTA = 0.0001;
const float bumpHeight = 10;

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
uniform int hasTexture = -1;
uniform int isBump = 1;
uniform float texGamma = 1; // suppose linear space
uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_TargetNormal;
/* etc */
uniform vec3 cameraPos;
uniform float gamma = 1; 

mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(tUv), st2 = dFdy(tUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
	vec3 tbnN=vec3(0);
	vec3 N = normalize(wNor);
	mat3 TBN = getTBN( N );
	if( hasTexture>0 ) {
		if( isBump>0 ) {
			float Bu = texture(map_Bump0, tUv+vec2(TEX_DELTA,0)).r
							- texture(map_Bump0, tUv+vec2(-TEX_DELTA,0)).r;
			float Bv = texture(map_Bump0, tUv+vec2(0,TEX_DELTA)).r
							- texture(map_Bump0, tUv+vec2(0,-TEX_DELTA)).r;
			vec3 bumpVec = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1);
			N = normalize(TBN*bumpVec);
		}
		else {
			vec3 tsNor = texture(map_Bump0, tUv).rgb;
			N = normalize(TBN*tsNor);
		}
	}
	vec3 targetN = texture(map_TargetNormal, tUv).rgb;
	targetN = 2*targetN-1;
	mat3 targetTBN = getTBN( normalize(targetN) );
	tbnN = normalize(transpose(targetTBN)*N);

	vec3 outColor = tbnN*0.5+0.5;

	FragColor = vec4(outColor, 1);
}
