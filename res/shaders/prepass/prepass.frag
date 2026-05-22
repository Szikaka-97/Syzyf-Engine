#version 460
in vec3 viewNormal;

layout (location = 1) out vec3 outNormal; 

void main() {
    outNormal = normalize(viewNormal);
}
