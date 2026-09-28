#version 460 core

// Position, color and texture coordinate arrive through attribute locations 0, 1 and 2, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 uv;

// Handed to the fragment shader, which receives them interpolated across the triangle.
out vec3 vColor;
out vec2 vUV;

void main() {
    gl_Position = vec4(pos, 1.0);
    vColor = col;
    vUV = uv;
}
