#version 460 core

in vec2 vUV;

out vec4 color;

layout(binding = 0) uniform sampler2D scene; // HDR: linear values, can be above 1.0
layout(binding = 1) uniform sampler2D bloom; // the blurred scene, at half size

// Tone map values, matching Renderer::ToneMap.
const int CLIP = 0;
const int REINHARD = 1;
const int ACES = 2;

uniform bool bloomEnabled;
uniform float bloomStrength; // how much of the bloom is mixed in
uniform float exposure;      // multiplies the scene before the tone map
uniform int toneMap;
uniform float gamma;         // the player's brightness setting, 1.0 = neutral

// The filmic curve of the Academy Color Encoding System, as fitted by Krzysztof Narkowicz. It rolls bright
// values off softly towards 1.0, adds contrast and lowers the saturation of very bright colors, like film.
vec3 aces(vec3 x) {
    return clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
}

// The exact sRGB curve: a short straight line near black, then a 1/2.4 power.
vec3 linearToSrgb(vec3 c) {
    return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

void main() {
    vec3 c = texture(scene, vUV).rgb;

    // Bloom: a small part of the blurred light over the whole image. Only bright areas give a visible glow.
    if (bloomEnabled)
        c = mix(c, texture(bloom, vUV).rgb, bloomStrength);

    // Tone map: HDR down to the 0..1 that the window can show.
    c = max(c * exposure, 0.0);

    if (toneMap == REINHARD)
        c = c / (1.0 + c);
    else if (toneMap == ACES)
        c = aces(c);
    else
        c = min(c, 1.0); // clip: everything above white is white

    // Gamma, then the sRGB encoding for the screen. Done here, not with GL_FRAMEBUFFER_SRGB: that needs an
    // sRGB window, and the driver can give a window without one (e.g. after a switch to full screen).
    c = pow(c, vec3(1.0 / gamma));
    color = vec4(linearToSrgb(c), 1.0);
}
