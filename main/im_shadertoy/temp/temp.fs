#version 410 core
layout(location=0) out vec4 FragColor;

uniform vec3      iResolution;           // viewport resolution (in pixels)
uniform float     iTime;                 // shader playback time (in seconds)
uniform float     iTimeDelta;            // render time (in seconds)
uniform float     iFrameRate;            // shader frame rate
uniform int       iFrame;                // shader playback frame
uniform float     iChannelTime[4];       // channel playback time (in seconds)
uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
uniform sampler2D iChannel0;             // now only sampler2D,     todo:input channel. XX = 2D/Cube
uniform sampler2D iChannel1;             // now only sampler2D,     todo:input channel. XX = 2D/Cube
uniform sampler2D iChannel2;             // now only sampler2D,     todo:input channel. XX = 2D/Cube
uniform sampler2D iChannel3;             // now only sampler2D,     todo:input channel. XX = 2D/Cube
uniform vec4      iDate;                 // (year, month, day, time in seconds)
uniform float     iSampleRate;           // sound sample rate (i.e., 44100)




// 위에가 진짜 회색

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float gray = uv.x;
    if(uv.y>0.5)
        gray = pow(gray, 1/2.2);
    fragColor = vec4(vec3(gray), 1);
}




void main()
{
    mainImage(FragColor, vec2(gl_FragCoord.x, gl_FragCoord.y));
} 