#version 460 core

// Position, normal and texture coordinate arrive through attribute locations 0, 1 and 2, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

// Handed to the fragment shader (position and normal in world space), interpolated across the triangle.
out vec3 vPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vLightSpacePos; // position as the sun's shadow map sees it

// The data of each drawn copy (instance), in one storage buffer for the whole frame. One draw call draws many
// copies of a mesh, and gl_BaseInstance + gl_InstanceID says which copy this vertex belongs to.
struct Instance {
    mat4 model;        // object -> world: where this copy is placed
    mat4 normalMatrix; // rotates normals into world space: transpose(inverse(model)), from the CPU. Only the mat3 part is used.
};

layout(std430, binding = 0) readonly buffer Instances {
    Instance instances[];
};

// Each matrix moves the vertex into the next space. Applied right to left, so model goes first.
uniform mat4 view;       // world -> camera: the world as seen from the camera
uniform mat4 projection; // camera -> clip space: perspective, far things get smaller

uniform mat4 lightSpace; // world -> the sun's clip space, for shadow lookups

void main() {
    Instance instance = instances[gl_BaseInstance + gl_InstanceID];
    vec4 worldPos = instance.model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    vPos = vec3(worldPos);
    vNormal = mat3(instance.normalMatrix) * normal;
    vUV = uv;
    vLightSpacePos = lightSpace * worldPos;
}
