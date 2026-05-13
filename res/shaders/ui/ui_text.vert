#version 460 core
layout (location = 0) in vec3 aPos;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 projection;
uniform vec4 uvRectangle;

void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);

    TexCoords = vec2(
        aPos.x * uvRectangle.z + uvRectangle.x,
        aPos.y * uvRectangle.w + uvRectangle.y
    );
}
