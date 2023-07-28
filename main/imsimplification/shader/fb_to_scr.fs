//
// gamma collection with glEnable(...SRGB)
//

#version 330 core
out vec4 FragColor;

in vec2 tUv;

uniform sampler2D screenTex;

void main()
{
	// todo : uv 말고 vPos로 샘플링
    vec3 col = texture(screenTex, tUv).rgb;
    FragColor = vec4(col, 1.0);
} 