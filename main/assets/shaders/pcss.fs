#version 460 core
layout(location=0) out vec4 FragColor;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;
in vec4 lPos;

uniform vec3 cam_Pos;
struct LightDirectional {
	vec3 Pos;
	vec3 Dir;
	vec3 Color;
	float Intensity;
};
uniform LightDirectional lit;

struct ShadowDirectional {
	bool Enabled;
	float ZFar;
	vec2 TexelSize;
	vec2 OrthoSize;
	vec2 RadiusUv;
	float Bias;
};
uniform ShadowDirectional shadow;
uniform sampler2D map_Shadow;

uniform float gamma = 2.2;

vec3 N, L;
float NDL;
float NDDL;


const int nr_poisson = 32;
const vec2 poisson32[] = {
	{-0.975402, -0.0711386},
	{-0.920347, -0.41142},
	{-0.883908, 0.217872},
	{-0.884518, 0.568041},
	{-0.811945, 0.90521},
	{-0.792474, -0.779962},
	{-0.614856, 0.386578},
	{-0.580859, -0.208777},
	{-0.53795, 0.716666},
	{-0.515427, 0.0899991},
	{-0.454634, -0.707938},
	{-0.420942, 0.991272},
	{-0.261147, 0.588488},
	{-0.211219, 0.114841},
	{-0.146336, -0.259194},
	{-0.139439, -0.888668},
	{0.0116886, 0.326395},
	{0.0380566, 0.625477},
	{0.0625935, -0.50853},
	{0.125584, 0.0469069},
	{0.169469, -0.997253},
	{0.320597, 0.291055},
	{0.359172, -0.633717},
	{0.435713, -0.250832},
	{0.507797, -0.916562},
	{0.545763, 0.730216},
	{0.56859, 0.11655},
	{0.743156, -0.505173},
	{0.736442, -0.189734},
	{0.843562, 0.357036},
	{0.865413, 0.763726},
	{0.872005, -0.927},
};
float shadowing01() // Soft Shadow
{
	if(!shadow.Enabled) 
		return 1.f;
	vec3 shadow_clip_pos = lPos.xyz/lPos.w;
	vec3 shadow_tex_pos = (shadow_clip_pos+1.0)*0.5f;
	float cur_depth = shadow_tex_pos.z;
	// From: https://cwyman.org/papers/i3d14_adaptiveBias.pdf
	float bias = shadow.Bias*(1.0-NDDL) + shadow.RadiusUv.y*0.001;
	bias = max(bias, 0.0001);
	int nr_front = 0;
	for( int i=0; i<nr_poisson; i++ ) {
		vec2 off = 0.01 * shadow.RadiusUv * shadow.OrthoSize * poisson32[i];
		float front_depth = texture(map_Shadow, shadow_tex_pos.xy+off).r;
		if( cur_depth < front_depth + bias || front_depth==1.0 ) {
			nr_front++;
		}
	}
	// return texture(map_Shadow, shadow_tex_pos.xy).r;
	return float(nr_front)/float(nr_poisson);
}



void main()
{    
    L = normalize(lit.Pos - wPos);
    // vec3 N = normalize (cross (dFdx(wPos.xyz), dFdy(wPos.xyz)));
	N = normalize(wNor);
	NDL = max(dot(N, L),0);
	NDDL = max(dot(N, lit.Dir), 0);
	float shading = shadowing01() * NDL;
    vec3 outColor = vec3(shading);
    
    outColor = pow(outColor, vec3(1/gamma));
    FragColor = vec4(outColor, 1);
}
