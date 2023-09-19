#version 410 core
layout(location=0) out vec4 fragColor;

in vec2 texCoord;

uniform sampler2D tex;
uniform float gamma = 2.2; 

void main()
{
    vec3 outColor = texture(tex, texCoord).rgb;

	outColor = pow(outColor, vec3(1/gamma));

    fragColor = vec4(outColor, 1.0);
} 