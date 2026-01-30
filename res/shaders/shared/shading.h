#ifndef SHADER_SHADING_H

vec3 getLightStrength(in Light light, in vec3 worldPos);

#ifdef SHADING_LAMBERT

#define SHADING_FUNCTION shadeLambert

struct Material {
	vec3 diffuseColor;
	float diffuseStrength;
};

Material defaultMaterial() {
	Material mat;

	mat.diffuseColor = vec3(0, 0, 0);
	mat.diffuseStrength = 1.0;

	return mat;
}

vec3 shadeLambert(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type != DIRECTIONAL_LIGHT ? normalize(light.position - worldPos) : -light.direction;

	return mat.diffuseColor * mat.diffuseStrength * (getLightStrength(light, worldPos) * max(dot(lightDirection, normal), 0.0));
}

#endif

#ifdef SHADING_PHONG

#define SHADING_FUNCTION shadePhong

struct Material {
	vec3 diffuseColor;
	vec3 specularColor;
	float diffuseStrength;
	float specularStrength;
	float specularHighlight;
};

Material defaultMaterial() {
	Material mat;

	mat.diffuseColor = vec3(0, 0, 0);
	mat.specularColor = vec3(0, 0, 0);
	mat.diffuseStrength = 1.0;
	mat.specularStrength = 1.0;
	mat.specularHighlight = 1.0;

	return mat;
}

vec3 shadePhong(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type != DIRECTIONAL_LIGHT ? normalize(light.position - worldPos) : -light.direction;
	vec3 viewDirection = normalize(worldPos - Global_CameraWorldPos);
	vec3 reflected = reflect(lightDirection, normal);

	float d = dot(reflected, viewDirection);
	d = clamp(d, 0.0, 1.0);

	vec3 diffuseLight = mat.diffuseColor * (mat.diffuseStrength * getLightStrength(light, worldPos) * max(dot(lightDirection, normal), 0.0));
	vec3 specularLight = mat.specularColor * (mat.specularHighlight <= 0.0 ? 0 : (mat.specularStrength * pow(d, mat.specularHighlight)));

	return diffuseLight + specularLight;
}

#endif

#ifdef SHADING_PBR

#define SHADING_FUNCTION shadePBR
#define IGNORE_AMBIENT

struct Material {
	vec3 albedo;
	float metallic;
	float roughness;
};

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness*roughness;
	float a2 = a*a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float nom   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = 3.14159265359 * denom * denom;

	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 shadePBR(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 N = normal;
	vec3 V = normalize(Global_CameraWorldPos - worldPos);
	vec3 R = reflect(-V, N); 

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, mat.albedo, mat.metallic);

	vec3 L = light.type != DIRECTIONAL_LIGHT ? normalize(light.position - worldPos) : -light.direction;
	vec3 H = normalize(V + L);
	float distance = light.type != DIRECTIONAL_LIGHT ? length(light.position - worldPos): 9999999;
	vec3 radiance = getLightStrength(light, worldPos);

	float NDF = DistributionGGX(N, H, mat.roughness);
	float G   = GeometrySmith(N, V, L, mat.roughness);
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
	vec3 specular = numerator / denominator;

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - mat.metallic;

	float NdotL = max(dot(N, L), 0.0);
	return vec3(kD * mat.albedo / 3.1415 + specular) * radiance * NdotL;
}

#endif

#define SHADER_SHADING_H
#endif