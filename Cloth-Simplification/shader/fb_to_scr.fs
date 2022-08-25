#version 330 core
out vec4 FragColor;

in vec2 texCoords;

uniform sampler2D screenTex;

void main()
{
    vec3 col = texture(screenTex, texCoords).rgb;
    FragColor = vec4(col, 1.0);
} 