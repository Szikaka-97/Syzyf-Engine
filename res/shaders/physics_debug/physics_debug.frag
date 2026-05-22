#version 460

out vec4 fragColor;

in vec3 pColor;

#pragma no_depth_prepass
#pragma no_shadows

void main() {
	fragColor = vec4(pColor, 1.0);
}
