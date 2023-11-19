#version 410 core
layout(location=0) out vec4 FragColor;

in vec2 texCoord;

uniform sampler2D tex;
uniform vec3 inputGamma;
uniform vec3 outputGamma = vec3(2.2);
uniform int nrChannels=3;

uniform mat3 RGB2PCS = mat3(1);
uniform mat3 chromaticAdaptation = mat3(1); // von kries
uniform mat3 PCS2RGB = mat3(1); 

// 2.4일때 sRGB gamma function 사용
float linearToGamma(float u, float gamma) {
	if( abs(gamma-2.4)<0.0001 ) {
		float u_= abs(u);
		return  u_>0.0031308 ? ( sign(u)*1.055*pow(u_,0.41667)-0.055 ) : (12.92*u);
	}
	return sign(u)*pow(abs(u),1/gamma);
}

float gammaToLinear(float u, float gamma) {
	if( abs(gamma-2.4)<0.0001 ) {
		float u_ = abs(u);
		return u_>0.04045 ? (sign(u)*pow((u_+0.055)/1.055,2.4)):(u/12.92);
 	}
 	return sign(u)*pow(abs(u),gamma);
}

void main()
{
    vec4 texColor = texture(tex, texCoord);
	vec3 color = texColor.rgb;
	vec3 outColor;

	if( nrChannels==1 ) color = texColor.rrr;

	color = vec3(gammaToLinear(color.r, inputGamma.r),
				 gammaToLinear(color.g, inputGamma.g),
				 gammaToLinear(color.b, inputGamma.b));
	
	color = RGB2PCS * color;
	color = chromaticAdaptation * color; 
	color = PCS2RGB * color;

	color = vec3(linearToGamma(color.r, outputGamma.r),
				 linearToGamma(color.g, outputGamma.g),
				 linearToGamma(color.b, outputGamma.b));
	
	// debug
	//outColor = vec3(1);
	//outColor = texColor.xyz;
	outColor = color;

	FragColor = vec4(outColor, texColor.w);
	return;
}