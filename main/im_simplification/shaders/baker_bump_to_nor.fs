/*


*/

#version 410 core
layout(location=0) out vec4 FragColor;

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
const int MF_Opacity    = 1<<8;
const int MF_MR         = 1<<9;
const int MF_ARM        = 1<<1;
const int MF_SHININESS  = 1<<1;

uniform int map_Flags = 0;
uniform sampler2D map_Bump;
uniform float texDelta = 0.0001;
uniform float bumpHeight = 100;


// Tangent, Bitangent, Normal
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
	vec3 N = normalize(wNor); // Todo: faceNor을 출력해야하나?
	mat3 TBN = getTBN( N );
    vec3 detailN;
	
	if( (map_Flags & MF_NOR) > 0 ) {
		vec3 tsNor = texture(map_Bump, mUv).rgb;
		detailN = normalize(TBN*tsNor);
	}
	else if( (map_Flags & MF_HEIGHT) > 0 ) {
		float Bu = texture(map_Bump, mUv+vec2(texDelta,0)).r
						- texture(map_Bump, mUv+vec2(-texDelta,0)).r;
		float Bv = texture(map_Bump, mUv+vec2(0,texDelta)).r
						- texture(map_Bump, mUv+vec2(0,-texDelta)).r;
		vec3 bumpVec = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1); // Todo: tangent space에서 노멀라이즈와 월드에서 노멀라이즈 비교
		detailN = normalize(TBN*bumpVec);
	}

    vec3 tsDetailN = transpose(TBN)*detailN;
	
	vec3 outColor = tsDetailN*0.5+0.5;

    FragColor = vec4(outColor, 1);
}
