#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 projection;

out vec2 TexCoords;

void main() {
    TexCoords = aPos.xy;
    TexCoords.y = 1.0 - TexCoords.y;

    gl_Position = projection * model * vec4(aPos, 1.0);
}
