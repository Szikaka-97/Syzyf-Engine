#version 460

in vec2 pUVCoords;

uniform sampler2D colorTex;

out vec4 fragColor;

void main() {
	fragColor = texture(colorTex, pUVCoords);

	float gamma = 2.2;
	fragColor.rgb = pow(fragColor.rgb, vec3(1.0/gamma));
}
