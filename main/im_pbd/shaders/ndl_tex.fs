#version 410 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;
uniform float gamma = 2.2;

uniform bool enable_Tex;
uniform sampler2D tex;

void main()
{    
    vec3 L = normalize(lit.Pos - wPos);
    // vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
    vec3 N = (gl_FrontFacing)?normalize(wNor):normalize(-wNor);


    float shade = max(dot(N, L),0);

    vec3 baseColor;
    if (enable_Tex)
        baseColor = texture(tex, mUv).xyz;
    else
        baseColor = vec3(1);
    vec3 outColor = shade * baseColor;
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
