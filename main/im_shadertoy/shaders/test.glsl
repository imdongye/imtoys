// 위에가 진짜 회색

void change(out vec3 color) {
    if(color.r>0.5) {
        color = vec3(0);
        return;
    }
    color = vec3(1);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float gray = uv.x;
    if(uv.y>0.5)
        gray = pow(gray, 1/2.2);
    vec3 color = vec3(1,0,0);
    change(color);
    fragColor = vec4(vec3(color), 1);
}