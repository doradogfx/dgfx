#version 460 core

in vec3 vDir;

out vec4 color;

layout(binding = 0) uniform samplerCube sky;

// The sun disk, drawn over the sky at full resolution. Only with a procedural sky.
uniform bool sunDisk;
uniform vec3 toSun;       // unit direction to the sun
uniform vec3 sunColor;    // the sunlight that reaches the ground, HDR
uniform float sunCosSize; // cos of the disk's radius

void main() {
    vec3 dir = normalize(vDir);
    vec3 c = texture(sky, dir).rgb;

    if (sunDisk) {
        // A soft edge over a small part of the radius. The disk is brighter than the sky, so it glows with bloom,
        // but not so bright that the glow hides the disk.
        float edge = (1.0 - sunCosSize) * 0.2;
        c += sunColor * 4.0 * smoothstep(sunCosSize - edge, sunCosSize + edge, dot(dir, toSun));
    }

    color = vec4(c, 1.0);
}
