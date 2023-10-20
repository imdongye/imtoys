/*

2023fall advenced rendering theorem 2 

*/
#version 410
out vec4 FragColor;
const float PI = 3.1415926535;

in vec3 wPos;
in vec3 wNor;
in vec2 mUv;

uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform vec3 Tf;

uniform float d;
uniform float Tr;
uniform float Ns;
uniform float Ni;
uniform float roughness;

uniform int map_Flags;

uniform sampler2D map_Kd;
uniform sampler2D map_Ks;
uniform sampler2D map_Ka;
uniform sampler2D map_Ns;
uniform sampler2D map_Bump;
uniform float texDelta;
uniform float bumpHeight;


//***************************************************
//            Color Space Conversion Functions
//***************************************************
float tonemap_sRGB(float u) {
	float u_ = abs(u);
	return  u_>0.0031308?( sign(u)*1.055*pow( u_,0.41667)-0.055):(12.92*u);
}
float inverseTonemap_sRGB(float u) { 
	float u_ = abs(u);
	return u_>0.04045?(sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);
}
// gamma correction
vec3 tonemap( vec3 rgb, mat3 csc, float gamma ){
	vec3 rgb_ = csc*rgb;
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return vec3( tonemap_sRGB(rgb_.r), tonemap_sRGB(rgb_.g), tonemap_sRGB(rgb_.b) );
	return sign(rgb_)*pow( abs(rgb_), vec3(1./gamma) );
}
// linearize 
vec3 inverseTonemap( vec3 rgb, mat3 csc, float gamma ){
	if( abs( gamma-2.4) <0.01 ) // sRGB
		return csc*vec3( inverseTonemap_sRGB(rgb.r), inverseTonemap_sRGB(rgb.g), inverseTonemap_sRGB(rgb.b) );
	return csc*sign(rgb)*pow( abs(rgb), vec3(gamma) );
}



vec3 PhongBRDF(vec3 w_i, vec3 w_o, vec3 N ) {

}

vec3 BlinnPhongBRDF( vec3)

vec3 brdf( vec3 w_i, vec3 w_0, vec3 N, vec3 albedo ) {
 
}
 
void main() {
    vec3 faceN = normalize( cross( dFdx(worldPos), dFdy(worldPos) ) );
	vec3 N = normalize(normal);
	vec3 toLight = lightPosition-worldPos;
	vec3 w_i = normalize( toLight );
	vec3 w_o = normalize( cameraPosition - worldPos );
	if( dot(N,faceN) <0 ) N = -N;
	vec4 albedo = vec4(1);
	if( diffTexEnabled>0 )
		albedo = texture( diffTex, texCoord );
	vec3 Li = lightColor/dot(toLight,toLight);
	vec4 color;
	color.rgb = albedo.rgb * Li;
	color.a = albedo.a;
	outColor = vec4(tonemap(color.rgb,mat3(1),2.4),color.a);
}