#version 460 core

out vec4 color;

uniform vec3 lightColor;
uniform float brightness; // above 1 the lamp is brighter than white, so it glows with bloom

// The lamp itself isn't lit by anything, it just shows the light's color.
void main() {
    color = vec4(lightColor * brightness, 1.0);
}
