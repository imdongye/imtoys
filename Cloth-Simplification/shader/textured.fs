#version 410 core
out vec4 FragColor;

const float PI = 3.1415926;
const float TEX_DELTA = 0.0001;
const float bumpHeight = 15;

in vec3 wPos;
in vec3 wNor;
in vec3 vNor;
in vec2 tUv;

uniform mat4 viewMat = mat4(1);
uniform vec3 lightPos = vec3(100,100,100);
uniform vec3 lightDir = vec3(-1,-1,-1);
uniform float lightInt = 0.8;
uniform float ambInt = 0.1;
uniform int shininess = 20;

uniform int shadowEnabled = -1;

uniform vec3 cameraPos;

uniform sampler2D map_Kd0;
uniform sampler2D map_Bump0;
uniform sampler2D map_Ks0;

mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(tUv), st2 = dFdy(tUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
	vec3 N = normalize(wNor);
	if( vNor.z < 0 ) N = -N;
	mat3 TBN = getTBN( N );
	float Bu = texture(map_Bump0, tUv+vec2(TEX_DELTA,0)).r
					- texture(map_Bump0, tUv+vec2(-TEX_DELTA,0)).r;
	float Bv = texture(map_Bump0, tUv+vec2(0,TEX_DELTA)).r
					- texture(map_Bump0, tUv+vec2(0,-TEX_DELTA)).r;
	vec3 bumpVec = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1);
	N = normalize(TBN*bumpVec);

	vec3 dl = wPos - lightPos;
	vec3 L = normalize(-dl);
	vec3 V = normalize(cameraPos - wPos);
	vec3 R = 2*dot(N,L)*N-L;


	float visibility = 1.0;
	if( shadowEnabled > 0 )
	{
		// ...
	}

	vec4 albelo = texture(map_Kd0, tUv);

	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lambertian*albelo.rgb*lightInt;
	vec3 ambient = albelo.rgb*ambInt;
	vec3 specular = pow(max(0,dot(R,V)), shininess) * lambertian * vec3(1);

	FragColor = vec4(diffuse+ambient+specular, 1);
}
