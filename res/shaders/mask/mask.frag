#version 460
layout (location = 1) out uint outMask;

uniform uint maskFlags;

void main() {
    outMask = maskFlags;
}
