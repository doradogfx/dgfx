#version 460 core

in vec2 vUV;

out vec4 color;

// binding = N reads from texture unit N, so the app doesn't need to set these uniforms.
layout(binding = 0) uniform sampler2D tex0;
layout(binding = 1) uniform sampler2D tex1;

void main() {
    vec4 crate = texture(tex0, vUV);
    vec4 face = texture(tex1, vUV);
    
    color = mix(crate, face, 0.2);
}
