/*

원본의 노멀을 simplify된 normal의 tangent space로 보내서 노멀맵을 만든다.

*/

#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform sampler2D map_OriNormal;


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
	vec3 simpN = normalize( wNor );
	mat3 simpTBN = getTBN( simpN );

	vec3 oriN = texture(map_OriNormal, mUv).rgb;
	oriN = 2*oriN-1;

	vec3 simpTangentSpaceOri = transpose(simpTBN)*oriN;

	vec3 outColor = simpTangentSpaceOri*0.5+0.5;

	fragColor = vec4(outColor, 1);
}
