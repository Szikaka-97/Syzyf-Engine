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

#define SHADER_SHADING_H
#endif