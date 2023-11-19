// 위에가 진짜 회색

void mainImage( out vec4 FragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float gray = uv.x;
    if(uv.y>0.5)
        gray = pow(gray, 1/2.2);
    FragColor = vec4(vec3(gray), 1);
}