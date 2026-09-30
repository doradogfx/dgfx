#version 460 core

out vec2 vUV;

// Full-screen triangle with no vertex buffer: vertex IDs 0, 1, 2 become (-1,-1), (3,-1), (-1,3).
// That one oversized triangle covers the whole [-1,1] screen; the parts outside are clipped away.
// UVs follow the same pattern, so the screen maps to [0,1].
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vUV = p;
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
