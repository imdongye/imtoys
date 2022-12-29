#version 410 core
out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex;
uniform vec3 inputGamma;
uniform vec3 outputGamma = vec3(2.2);
uniform int nrChannels=3;
uniform mat3 rgbToPCS = mat3(1); // linearRGB*XYZ
uniform mat3 chromaticAdaptation = mat3(1); // von kries
uniform mat3 PCSToDisplay = mat3(1); // linearRGB*RGB

float tonemap(float u, float gamma) {
	if( abs(gamma-2.4)<0.0001 ) {
		float u_= abs(u);
		return  u_>0.0031308 ? ( sign(u)*1.055*pow(u_,0.41667)-0.055 ) : (12.92*u);
	}
	return sign(u)*pow(abs(u),1/gamma);
}

float invTonemap(float u, float gamma) {
	if( abs(gamma-2.4)<0.0001 ) {
		float u_ = abs(u);
		return u_>0.04045?(sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);
 	}
 	return sign(u)*pow(abs(u),gamma);
}

void main()
{
    vec4 color = texture(tex, texCoord);

	if( nrChannels==1 ) color.rgb = color.rrr;

	color.rgb = rgbToPCS * color.rgb;
	color.rgb = chromaticAdaptation * color.rgb; 
	color.rgb = PCSToDisplay * color.rgb;


    FragColor = color;
} 