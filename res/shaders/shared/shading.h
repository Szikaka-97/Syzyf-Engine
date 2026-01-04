#ifndef SHADER_SHADING_H

vec3 getLightStrength(in Light light, in vec3 worldPos);

#ifdef SHADING_LAMBERT

#define SHADING_FUNCTION shadeLambert

struct Material {
	vec3 diffuseColor;
	float diffuseStrength;
};

vec3 shadeLambert(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type != DIRECTIONAL_LIGHT ? normalize(light.position - worldPos) : -light.direction;

	return mat.diffuseColor * (getLightStrength(light, worldPos) * max(dot(lightDirection, normal), 0.0));
}

#endif

#ifdef SHADING_PHONG

#define SHADING_FUNCTION shadeLambert

struct Material {
	vec3 diffuseColor;
	vec3 specularColor;
	float diffuseStrength;
	float specularStrength;
	float specularHighlight;
};

vec3 shadePhong(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type != DIRECTIONAL_LIGHT ? normalize(light.position - worldPos) : -light.direction;
	vec3 viewDirection = normalize(worldPos - Global_CameraWorldPos);
	vec3 reflected = reflect(lightDirection, normal);

	float d = dot(reflected, viewDirection);
	d = clamp(d, 0.0, 1.0);

	vec3 diffuseLight = getLightStrength(light, worldPos) * mat.diffuseColor * max(dot(lightDirection, normal), 0.0);
	// vec3 specularLight = mat.specularColor * pow(dot(viewDirection, reflected), mat.specularHighlight);
	vec3 specularLight = mat.specularColor * pow(d, 1);

	// if (specularLight.x < 0.99) {
	// 	specularLight = vec3(0, 0, 0);
	// }

	return diffuseLight;
}

#endif

#define SHADER_SHADING_H
#endif