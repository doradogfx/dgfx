#version 460 core

// Position, normal and texture coordinate arrive through attribute locations 0, 1 and 2, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

// Handed to the fragment shader (position and normal in world space), interpolated across the triangle.
out vec3 vPos;
out vec3 vNormal;
out vec2 vUV;

// Each matrix moves the vertex into the next space. Applied right to left, so model goes first.
uniform mat4 model;      // object -> world: where this object is placed
uniform mat4 view;       // world -> camera: the world as seen from the camera
uniform mat4 projection; // camera -> clip space: perspective, far things get smaller

// Rotates normals into world space. Computed on the CPU as transpose(inverse(model))
uniform mat3 normalMatrix;

void main() {
    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
    vPos = vec3(worldPos);
    vNormal = normalMatrix * normal;
    vUV = uv;
}
