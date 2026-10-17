#version 460 core

// One step up the bloom chain: a 3x3 tent filter over the smaller level. Additive blending adds the result onto
// this level, so each level gets the wide, soft light of all the smaller ones.

in vec2 vUV;

out vec3 color;

layout(binding = 0) uniform sampler2D source; // the level below, half the size of this one

uniform float radius; // the filter's reach in UV. Larger = wider glow.

void main() {
    vec2 r = vec2(radius);

    // Weights 1-2-1 in each direction, 16 in total.
    vec3 sum = texture(source, vUV + vec2(-r.x,  r.y)).rgb
             + texture(source, vUV + vec2( 0.0,  r.y)).rgb * 2.0
             + texture(source, vUV + vec2( r.x,  r.y)).rgb
             + texture(source, vUV + vec2(-r.x,  0.0)).rgb * 2.0
             + texture(source, vUV).rgb * 4.0
             + texture(source, vUV + vec2( r.x,  0.0)).rgb * 2.0
             + texture(source, vUV + vec2(-r.x, -r.y)).rgb
             + texture(source, vUV + vec2( 0.0, -r.y)).rgb * 2.0
             + texture(source, vUV + vec2( r.x, -r.y)).rgb;

    color = sum / 16.0;
}
