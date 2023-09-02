#version 410 core
out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex;
uniform float gamma = 2.2; 

void main()
{
    vec3 outColor = vec3(texture(tex, texCoord).r);

	outColor = pow(outColor, vec3(1/gamma));

    FragColor = vec4(outColor, 1.0);
} 