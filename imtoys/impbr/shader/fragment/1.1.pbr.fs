#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform float ao;
/* edit runtime */
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform bool isGGX = true;
uniform vec3 emissivity = vec3(0);

// lights
uniform vec3 lightPosition;
uniform vec3 lightColor;

uniform vec3 camPos;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float BeckmannNDF(vec3 N, vec3 H, float roughness) 
{
	// AlphaB
	float a = roughness*roughness;
	float a2 = a*a;
	a2 = max(a2, 0.0000001);
	float NDH = dot(N, H);
    float NDH2 = NDH*NDH;
    float NDH4 = NDH2*NDH2;

	float D = 1/(PI*a2*NDH4);
	D = max(NDH, 0.0) / (PI*a2*NDH4); // in slide
	D *= exp( (NDH2-1)/(a2*NDH2) );

	return D;
}
// ----------------------------------------------------------------------------
// exponantiol이 없어서 계산비용적음
float BlinnPhongNDF(vec3 N, vec3 H, float roughness) 
{
	// AlphaP
	float a = roughness*roughness;
	float NDH = max(dot(N, H), 0.0);
	float NDH_P = pow(NDH, a); // NDH^alphaP

	float D = NDH;
	D *= (a+2)/(2*PI);
	D *= NDH_P;

	return D;
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	// AlphaG
    float a = roughness*roughness;
    float a2 = a*a;
    float NDH = max(dot(N, H), 0.0);
    float NDH2 = NDH*NDH;

    float nom = a2; 
	nom = NDH * a2; // in slide
    float denom = 1.0 + NDH2 * (a2 - 1.0);
    denom = PI * denom * denom;
	
	// prevent divide by zero for roughness=0.0 and NdotH=1.0
    return nom / max(denom, 0.0000001);
}
// ----------------------------------------------------------------------------
// 디즈니와 언리얼에서 예전에 사용함.
// Schlick-Beckmann Geometry Shadowing Function
float GeometrySchlickGGX(float NDX, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NDX;
    float denom = NDX * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NDV = max(dot(N, V), 0.0);
    float NDL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NDV, roughness); // masking
    float ggx1 = GeometrySchlickGGX(NDL, roughness); // shadowing

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
// S = V or L 
// => masking or shadowing
float LamdaBeckmann(vec3 N , vec3 S, float roughness)
{
	/* makeing a */
	// AlphaB
	float alpha = roughness*roughness;
	float NDS = dot(N, S);
	float NDS2 = NDS*NDS;
	float denom = alpha*sqrt(1.0-NDS2);
	float a = NDS/denom;

	/* approximation lambda with a */
	if(a<1.6) {
		float nom = 1.0 - 1.259*a + 0.396*a*a;
		float denom = 3.535*a + 2.181*a*a;
		return nom/denom;
	}
	return 0;
}
float G1Beckmann(vec3 H, vec3 V, float lambda)
{
	float VDH = dot(V, H);
	float denom = 1.0+lambda;

	return max(VDH, 0.0)/denom;
}
// 동작안함.
// from: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
float GeometryBeckmann(vec3 N, vec3 X, float roughness) 
{
	// AlphaB
	float a = roughness*roughness;
	float NDX = dot(N, X);
	float nom = NDX;
	float denom = a*sqrt(1.0-NDX*NDX);
	float c = nom/denom;
	if( c<1.6 ) {
		nom = 3.535*c+2.18*c*c;
		denom = 1.0+2.276*c+2.577*c*c;
		return nom/denom;
	}
	return 1;
}
// shading 이상함.
float G2Beckmann(vec3 N, vec3 V, vec3 L, float roughness) 
{
	vec3  H = normalize(V+L);
	float masking = G1Beckmann(H, V, LamdaBeckmann(N, V, roughness));
	float shadowing = G1Beckmann(H, V, LamdaBeckmann(N, L, roughness));
	//masking = GeometryBeckmann(H, V, roughness);
	//shadowing = GeometryBeckmann(H, L, roughness);
	return masking*shadowing;
}
// ----------------------------------------------------------------------------
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------


void main()
{		
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
	vec3 L = normalize(lightPosition - WorldPos);
    vec3 H = normalize(V + L); // Half-way vector i.o. m: Microfacit
    float dist = length(lightPosition - WorldPos);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = lightColor * attenuation;
	float NDV = dot(N,V);
	float NDL = dot(N,L); // lambertian


    // metallic하면 F0를 albedo로 사용함.
    vec3 F0 = vec3(0.04); // dielectric ex Plastics
    F0 = mix(F0, albedo, metallic);

	vec3 Ks = FresnelSchlick(clamp(dot(V, H), 0.0, 1.0), F0); // kS is equal to Fresnel
	vec3 Kd = vec3(1.0) - Ks;			// for energy conservation
	vec3 lambert = albedo/PI;

	// multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    Kd *= 1.0 - metallic;	    

    /* Cook-Torrance BRDF */
    float NDF = (isGGX) ? DistributionGGX(N, H, roughness) : BeckmannNDF(N, H, roughness);
	// beckmann shadowing term 망가짐.
    float GSF = (isGGX) ? GeometrySmith(N, V, L, roughness) : G2Beckmann(N, V, L, roughness);      
    vec3  FF  = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
    vec3  DGF   = NDF * GSF * FF; 
    float denom = 4 * max(NDV,0.0) * max(NDL,0.0);
    vec3  cookTorrance = DGF / max(denom, 0.001); // Ks: specular


    vec3 BRDF = Kd*lambert + cookTorrance;

	vec3 Lo = emissivity + BRDF * radiance * max(NDL,0.0);

    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

	// Debug
	//color = vec3(NDF);
	//color = vec3(GSF);
	//color = FF;


    color = color / (color + vec3(1.0)); // HDR tonemapping
    color = pow(color, vec3(1.0/2.2)); // gamma correct
    FragColor = vec4(color, 1.0);
}
