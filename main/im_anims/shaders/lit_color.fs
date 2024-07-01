#version 410 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;
uniform float gamma = 2.2;
uniform vec4 color;

void main()
{    
    vec3 L = normalize(lit.Pos - wPos);
	vec3 N = wNor;

    vec3 outColor = color.xyz * max(dot(N, L),0);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
