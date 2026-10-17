#version 460 core

// One step down the bloom chain: a 13-tap filter that reads the bigger level and writes this one at half the size.
// From "Next Generation Post Processing in Call of Duty: Advanced Warfare" (Jimenez, 2014).

in vec2 vUV;

out vec3 color;

layout(binding = 0) uniform sampler2D source; // the level above, 2x the size of this one

uniform vec2 sourceTexel;  // 1 / the size of the source
uniform bool firstLevel;   // reads the HDR scene: use the Karis average

// Brightness of a color, for the Karis average.
float luma(vec3 c) {
    return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

// The average of 4 samples, each weighted by 1 / (1 + brightness). A single very bright pixel then cannot make
// the whole block bright, which would flicker ("fireflies") as it moves.
vec3 karis(vec3 a, vec3 b, vec3 c, vec3 d) {
    float wa = 1.0 / (1.0 + luma(a));
    float wb = 1.0 / (1.0 + luma(b));
    float wc = 1.0 / (1.0 + luma(c));
    float wd = 1.0 / (1.0 + luma(d));
    return (a * wa + b * wb + c * wc + d * wd) / (wa + wb + wc + wd);
}

void main() {
    vec2 t = sourceTexel;

    // The 13 samples, in a pattern of an outer 4x4 grid (a..m) around the center e:
    //   a . b . c
    //   . j . k .
    //   d . e . f
    //   . l . m .
    //   g . h . i
    vec3 a = texture(source, vUV + t * vec2(-2,  2)).rgb;
    vec3 b = texture(source, vUV + t * vec2( 0,  2)).rgb;
    vec3 c = texture(source, vUV + t * vec2( 2,  2)).rgb;
    vec3 d = texture(source, vUV + t * vec2(-2,  0)).rgb;
    vec3 e = texture(source, vUV).rgb;
    vec3 f = texture(source, vUV + t * vec2( 2,  0)).rgb;
    vec3 g = texture(source, vUV + t * vec2(-2, -2)).rgb;
    vec3 h = texture(source, vUV + t * vec2( 0, -2)).rgb;
    vec3 i = texture(source, vUV + t * vec2( 2, -2)).rgb;
    vec3 j = texture(source, vUV + t * vec2(-1,  1)).rgb;
    vec3 k = texture(source, vUV + t * vec2( 1,  1)).rgb;
    vec3 l = texture(source, vUV + t * vec2(-1, -1)).rgb;
    vec3 m = texture(source, vUV + t * vec2( 1, -1)).rgb;

    // Five overlapping 2x2 boxes: the center one (j k l m) counts for half, the four corner ones for 1/8 each.
    if (firstLevel) {
        color = karis(j, k, l, m) * 0.5
              + karis(a, b, d, e) * 0.125 + karis(b, c, e, f) * 0.125
              + karis(d, e, g, h) * 0.125 + karis(e, f, h, i) * 0.125;
    } else {
        color = (j + k + l + m) * 0.125
              + (a + b + d + e + b + c + e + f + d + e + g + h + e + f + h + i) * 0.03125;
    }
}
