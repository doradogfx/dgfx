#version 460 core

// Position, color and texture coordinate arrive through attribute locations 0, 1 and 2, fed from the vertex buffer.
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 uv;

// Handed to the fragment shader, which receives them interpolated across the triangle.
out vec3 vColor;
out vec2 vUV;

// Moves/rotates/scales the vertices. Applied as matrix * vector, so the matrix goes on the left.
uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(pos, 1.0);
    vColor = col;
    vUV = uv;
}
