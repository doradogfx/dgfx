#version 460 core

in vec3 vDir;

out vec4 color;

layout(binding = 0) uniform samplerCube sky;

void main() {
    color = vec4(texture(sky, vDir).rgb, 1.0);
}
