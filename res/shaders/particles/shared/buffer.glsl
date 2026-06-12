struct ParticleData {
    vec4 position;
    // w is size
    vec4 velocity;
    vec4 lifetime;
};

layout(std430, binding = 3) buffer ParticleBuffer {
    ParticleData particles[];
};
