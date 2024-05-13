#version 410 core
out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform int map_Flags;


mat3 getTBN( vec3 N ) {
	vec3 Q1 = dFdx(wPos), Q2 = dFdy(wPos);
	vec2 st1 = dFdx(mUv), st2 = dFdy(mUv);
	float D = st1.s*st2.t - st2.x*st1.t;
	return mat3(normalize((Q1*st2.t - Q2*st1.t)*D),
				normalize((-Q1*st2.s + Q2*st1.s)*D), N);
}

void main(void)
{
    FragColor = vec4(0,0,0,1);
    if( (map_Flags&(1<<4)) > 0 ) {
	    FragColor += vec4(1,0,0,0); // bump
    }
    if( (map_Flags&(1<<5)) > 0 ) {
	    FragColor += vec4(0,1,0,0); // nor
    }
	if( (map_Flags&(1<<0)) > 0 ) {
	    FragColor += vec4(0,0,1,0); // Kd
    }
}
