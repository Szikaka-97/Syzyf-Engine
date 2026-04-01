#version 460

out vec4 fragColor;

in vec3 pColor;

void main() {
	fragColor = vec4(pColor, 1.0);
}
