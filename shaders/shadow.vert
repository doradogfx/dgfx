#version 460 core

layout(location = 0) in vec3 pos;

// The same instance buffer as lit.vert. See there.
struct Instance {
    mat4 model;
    mat4 normalMatrix;
};

layout(std430, binding = 0) readonly buffer Instances {
    Instance instances[];
};

uniform mat4 lightSpace;

void main() {
    gl_Position = lightSpace * instances[gl_BaseInstance + gl_InstanceID].model * vec4(pos, 1.0);
}
