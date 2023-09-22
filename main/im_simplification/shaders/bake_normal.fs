
#version 410 core
layout(location=0) out vec4 fragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

/* texture */
uniform int map_Flags = 0;
uniform sampler2D map_Bump;
uniform sampler2D map_TargetNormal;
uniform float texDelta = 0.0001;
uniform float bumpHeight = 100;


mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	vec3 T = normalize((Q1*st2.t - Q2*st1.t)*D);
	vec3 B = normalize((-Q1*st2.s + Q2*st1.s)*D);
	// gram schmidt
	T = normalize(T -dot(T,N)*N);
	B = cross(N, T);
	return mat3(T, B, N);
}

void main(void)
{
	vec3 targetN, targetSpaceN, N;
	mat3 targetTBN, TBN;
	N = normalize( wNor );
	TBN = getTBN( N );
	
	if( (map_Flags & 1<<5) > 0 ) {
		vec3 tsNor = texture(map_Bump, mUv).rgb;
		N = normalize(TBN*tsNor);
	}
	else if( (map_Flags & 1<<4) > 0 ) {
		float Bu = texture(map_Bump, mUv+vec2(texDelta,0)).r
						- texture(map_Bump, mUv+vec2(-texDelta,0)).r;
		float Bv = texture(map_Bump, mUv+vec2(0,texDelta)).r
						- texture(map_Bump, mUv+vec2(0,-texDelta)).r;
		vec3 bumpVec = vec3(-Bu*bumpHeight, -Bv*bumpHeight, 1);
		N = normalize(TBN*bumpVec);
	}

	targetN = texture(map_TargetNormal, mUv).rgb;
	targetN = normalize(2*targetN-1);
	targetTBN = getTBN( targetN );

	targetSpaceN = transpose(targetTBN)*N;

	vec3 outColor = targetSpaceN*0.5+0.5;

	fragColor = vec4(outColor, 1);
}
