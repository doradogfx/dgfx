#version 460 core

// The sky from sunlight scattered by the air (single scattering, Nishita 1993). Drawn into the faces of a cubemap.
// Two kinds of scattering:
//   Rayleigh: air molecules. Scatters blue much more than red: the blue sky, and the red sunset when the light
//             crosses much more air.
//   Mie:      haze and dust. Scatters all colors about the same, mostly forward: the bright glow around the sun.

in vec3 vDir;

out vec4 color;

uniform vec3 toSun;          // unit direction from the ground to the sun
uniform float sunIntensity;  // the sun's strength before the atmosphere
uniform float rayleighScale; // x the air's scattering
uniform float mieScale;      // x the haze

const float PI = 3.14159265;
const float EARTH_RADIUS = 6360e3;      // meters
const float ATMOSPHERE_RADIUS = 6420e3;
const float RAYLEIGH_HEIGHT = 7994.0;   // the air's density falls by e every this many meters up
const float MIE_HEIGHT = 1200.0;
const vec3 RAYLEIGH_BETA = vec3(5.8e-6, 13.5e-6, 33.1e-6); // scattering per meter at sea level, red green blue
const float MIE_BETA = 21e-6;
const float MIE_G = 0.76;               // how much the haze scatters forward (0 = the same in all directions)
const int VIEW_STEPS = 16;
const int SUN_STEPS = 8;

// Where the ray from o along d enters and leaves a sphere at the planet's center. x > y means no hit.
vec2 raySphere(vec3 o, vec3 d, float radius) {
    float b = dot(o, d);
    float c = dot(o, o) - radius * radius;
    float disc = b * b - c;

    if (disc < 0.0)
        return vec2(1.0, -1.0);

    float s = sqrt(disc);
    return vec2(-b - s, -b + s);
}

vec3 scatter(vec3 dir) {
    vec3 origin = vec3(0.0, EARTH_RADIUS + 1.0, 0.0); // the viewer, one meter above the ground
    float far = raySphere(origin, dir, ATMOSPHERE_RADIUS).y;
    vec2 ground = raySphere(origin, dir, EARTH_RADIUS);

    if (ground.x <= ground.y && ground.x > 0.0)
        far = ground.x; // the ray hits the ground first

    vec3 betaR = RAYLEIGH_BETA * rayleighScale;
    float betaM = MIE_BETA * mieScale;
    float step = far / float(VIEW_STEPS);
    float depthR = 0.0; // the air between the viewer and the sample, weighted by density
    float depthM = 0.0;
    vec3 sumR = vec3(0.0);
    vec3 sumM = vec3(0.0);

    for (int i = 0; i < VIEW_STEPS; i++) {
        vec3 p = origin + dir * (float(i) + 0.5) * step;
        float height = length(p) - EARTH_RADIUS;
        float densityR = exp(-height / RAYLEIGH_HEIGHT) * step;
        float densityM = exp(-height / MIE_HEIGHT) * step;
        depthR += densityR;
        depthM += densityM;

        // The air between the sample and the sun. The planet's shadow: no sunlight reaches the sample.
        vec2 planet = raySphere(p, toSun, EARTH_RADIUS);

        if (planet.x <= planet.y && planet.x > 0.0)
            continue;

        float sunStep = raySphere(p, toSun, ATMOSPHERE_RADIUS).y / float(SUN_STEPS);
        float sunDepthR = 0.0;
        float sunDepthM = 0.0;

        for (int j = 0; j < SUN_STEPS; j++) {
            float h = length(p + toSun * (float(j) + 0.5) * sunStep) - EARTH_RADIUS;
            sunDepthR += exp(-h / RAYLEIGH_HEIGHT) * sunStep;
            sunDepthM += exp(-h / MIE_HEIGHT) * sunStep;
        }

        // The light that is left after both paths through the air. Mie also absorbs a little (1.1).
        vec3 transmittance = exp(-(betaR * (depthR + sunDepthR) + betaM * 1.1 * (depthM + sunDepthM)));
        sumR += transmittance * densityR;
        sumM += transmittance * densityM;
    }

    // How much light turns from the sun's direction towards the viewer.
    float mu = dot(dir, toSun);
    float phaseR = 3.0 / (16.0 * PI) * (1.0 + mu * mu);
    float g2 = MIE_G * MIE_G;
    float phaseM = 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + mu * mu)) / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * MIE_G * mu, 1.5));

    return sunIntensity * (sumR * betaR * phaseR + sumM * betaM * phaseM);
}

void main() {
    vec3 dir = normalize(vDir);

    // Below the horizon: the haze at the horizon, fading darker further down, instead of the black ground of the
    // planet. A far ground is hidden by haze, and reflections that look down keep a plausible color.
    vec3 sky = dir.y >= 0.0 ? scatter(dir)
                            : scatter(normalize(vec3(dir.x, 0.0, dir.z))) * mix(1.0, 0.3, smoothstep(0.0, 0.4, -dir.y));

    // Night: a dark blue floor, so the sky never goes fully black.
    color = vec4(sky + vec3(0.0015, 0.002, 0.004), 1.0);
}
