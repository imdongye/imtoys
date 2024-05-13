//
// todo: 노멀맵에서 범프맵을 만들때 적분을 못해서 미분값을 가지고 포아송 방정식(자코비, 가우스세이델)을 풀면된다.
#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

const float PI = 3.1415926535;
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

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
struct ShadowDirectional {
	bool Enabled;
	float ZNear;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
};
uniform LightDirectional lit;
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;


mat3 getTBN( vec3 N ) {
	vec3 Qx = dFdx(wPos);
	vec3 Qy = dFdy(wPos);
	vec2 Tx = dFdx(mUv);
	vec2 Ty = dFdy(mUv);
	float D = Tx.x*Ty.y - Ty.x*Tx.y;
	vec3 T = normalize(( Qx*Ty.y - Qy*Tx.y)*D);
	vec3 B = normalize((-Qx*Ty.x + Qy*Tx.x)*D);
	T = normalize(T -dot(T,N)*N); // Todo: gram schmidt말고 다른방법으로 에러 줄이기
	B = cross(N, T);
	return mat3(T, B, N);
}

void main(void)
{
	vec3 N = normalize(wNor);
	vec3 L = normalize(lit.Pos - wPos);
	vec3 V = normalize(cam_Pos - wPos);

	if( (map_Flags&(MF_NOR|MF_HEIGHT)) > 0 ) // has bump
	{
		mat3 TBN = getTBN( N );
		vec3 tsNor; // tangent normal
		if( (map_Flags&MF_HEIGHT) > 0 ) { // bump map
			float Bu = texture(map_Bump, mUv+vec2(mat.TexDelta,0)).r
						- texture(map_Bump, mUv+vec2(-mat.TexDelta,0)).r;
			float Bv = texture(map_Bump, mUv+vec2(0,mat.TexDelta)).r
						- texture(map_Bump, mUv+vec2(0,-mat.TexDelta)).r;
			tsNor = vec3(-Bu*mat.BumpHeight, -Bv*mat.BumpHeight, 1);
		}
		else { // nor map
			tsNor = texture(map_Bump, mUv).xyz*2.0-vec3(1);
		}
		N = normalize(TBN*tsNor);
	}
	vec3 R = 2*dot(N,L)*N-L;
	

	if( shadow.Enabled )
	{
		// ...
	}

	vec3 albelo = ( (map_Flags&MF_BASE_COLOR) > 0 ) ? texture(map_BaseColor, mUv).rgb : mat.BaseColor;
	float lambertian = max(0, dot(N, L));
	vec3 diffuse = lit.Color*lambertian*albelo;
	vec3 ambient = albelo*0.2;//Ka;
	vec3 specular = pow(max(0,dot(R,V)), mat.Shininess) * lambertian * vec3(1);
	vec3 outColor = diffuse+ambient+specular;

    outColor = pow(outColor, vec3(1/2.2));
    FragColor = vec4(outColor, 1);
}
