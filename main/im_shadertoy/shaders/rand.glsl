float rand(int i) {
    vec4 seed4 = gl_FragCoord.zxyx;
    float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
    return fract(sin(float(i+123)*dot_product) * 42758.5453);
}

// iqint3 From: https://www.shadertoy.com/view/4tXyWN
float iqint3( uvec2 x ) {
    uvec2 q = 1103515245U * ( (x>>1U) ^ (x.yx   ) );
    uint  n = 1103515245U * ( (q.x  ) ^ (q.y>>3U) );
    return float(n) * (1.0/float(0xffffffffU));
}


float randWith2(vec2 co){
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void mainImage( out vec4 FragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    float col1 = rand(0);
    float col2 = iqint3(uvec2(gl_FragCoord.xy));
    float col3 = randWith2(vec2(gl_FragCoord.xy));
    float rst = mix(col1, col2, step(0.5, uv.x));
    rst = mix(rst, col3, step(0.5, uv.y));

    FragColor = vec4(rst);
}