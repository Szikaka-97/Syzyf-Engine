uniform float heightScale;
uniform float pomMinLayers;
uniform float pomMaxLayers;
uniform vec2 uvScale;

vec3 getNormalFromMap(vec2 uv, mat3 TBN) {
	vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;
	return normalize(TBN * tangentNormal);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    float numLayers = mix(pomMaxLayers, pomMinLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(armMap, currentTexCoords * uvScale).a;
    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(armMap, currentTexCoords * uvScale).a;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(armMap, prevTexCoords * uvScale).a - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
    return finalTexCoords;
}
