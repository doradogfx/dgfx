#version 460 core

// Position and color arrive through attribute locations 0 and 1, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

// Handed to the fragment shader, which receives it interpolated across the triangle.
out vec3 vColor;

void main() {
    gl_Position = vec4(pos, 1.0);
    vColor = col;
}
