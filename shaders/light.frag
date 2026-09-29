#version 460 core

out vec4 color;

uniform vec3 lightColor;

// The lamp itself isn't lit by anything, it just shows the light's color.
void main() {
    color = vec4(lightColor, 1.0);
}
