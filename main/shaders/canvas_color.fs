#version 410 core
layout(location=0) out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex;
uniform float gamma = 2.2; 
uniform float bias = 0.0;
uniform float gain = 1.0;

void main()
{
    vec3 outColor = texture(tex, texCoord).rgb;
    outColor = bias + gain*outColor;

	outColor = pow(outColor, vec3(1/gamma));

    FragColor = vec4(outColor, 1.0);
} 