#version 460 core

in vec2 vUV;

out vec4 color;

layout(binding = 0) uniform sampler2D scene; // linear values (sRGB texture, decoded when sampled)

// Effect values, matching PostEffect in main.cpp.
const int NONE = 0;
const int GRAYSCALE = 1;
const int INVERT = 2;
const int BLUR = 3;
const int SHARPEN = 4;
const int EDGES = 5;

uniform int effect;
uniform float gamma; // user brightness calibration, 1.0 = neutral

// Weighted sum of the 3x3 neighborhood around this pixel, kernel given row by row from the top.
vec3 convolve(float k[9]) {
    vec2 texel = 1.0 / vec2(textureSize(scene, 0));
    vec3 sum = vec3(0.0);

    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            vec2 offset = vec2(x - 1, 1 - y) * texel;
            sum += k[y * 3 + x] * texture(scene, vUV + offset).rgb;
        }
    }

    return sum;
}

void main() {
    vec3 c = texture(scene, vUV).rgb;

    if (effect == GRAYSCALE) {
        // Luminance: the eye is most sensitive to green, least to blue.
        c = vec3(dot(c, vec3(0.2126, 0.7152, 0.0722)));
    } else if (effect == INVERT) {
        c = 1.0 - c;
    } else if (effect == BLUR) {
        c = convolve(float[](1, 2, 1, 2, 4, 2, 1, 2, 1)) / 16.0;
    } else if (effect == SHARPEN) {
        c = convolve(float[](0, -1, 0, -1, 5, -1, 0, -1, 0));
    } else if (effect == EDGES) {
        c = convolve(float[](1, 1, 1, 1, -8, 1, 1, 1, 1));
    }

    c = pow(max(c, 0.0), vec3(1.0 / gamma));
    color = vec4(c, 1.0);
}
