#ifndef SHADER_SHADING_H

vec3 getLightStrength(in Light light, in vec3 worldPos);

vec3 shadeLambert(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type == POINT_LIGHT ? normalize(light.position - worldPos) : light.direction;

	return getLightStrength(light, worldPos) * mat.diffuseColor * dot(lightDirection, normal);
}

vec3 shadePhong(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 lightDirection = light.type == POINT_LIGHT ? normalize(light.position - worldPos) : light.direction;
	vec3 viewDirection = normalize(worldPos - Global_CameraWorldPos);
	vec3 reflected = reflect(lightDirection, normal);

	float d = dot(reflected, viewDirection);
	d = clamp(d, 0.0, 1.0);

	vec3 diffuseLight = getLightStrength(light, worldPos) * mat.diffuseColor * dot(lightDirection, normal);
	// vec3 specularLight = mat.specularColor * pow(dot(viewDirection, reflected), mat.specularHighlight);
	vec3 specularLight = mat.specularColor * pow(d, 1);

	// if (specularLight.x < 0.99) {
	// 	specularLight = vec3(0, 0, 0);
	// }

	return diffuseLight;
}

#define SHADER_SHADING_H
#endif