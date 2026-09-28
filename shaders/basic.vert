#version 460 core

// Position and texture coordinate arrive through attribute locations 0 and 1, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

// Handed to the fragment shader, which receives it interpolated across the triangle.
out vec2 vUV;

// Each matrix moves the vertex into the next space. Applied right to left, so model goes first.
uniform mat4 model;      // object -> world: where this object is placed
uniform mat4 view;       // world -> camera: the world as seen from the camera
uniform mat4 projection; // camera -> clip space: perspective, far things get smaller

void main() {
    gl_Position = projection * view * model * vec4(pos, 1.0);
    vUV = uv;
}
