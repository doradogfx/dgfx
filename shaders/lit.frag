#version 460 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vLightSpacePos;

out vec4 color;

// Colors are tints, multiplied with what the maps return at this pixel.
// Solid-color materials get a 1x1 white map, leaving just the tint.
struct Material {
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

// Outside the struct: layout(binding) isn't allowed on struct members.
layout(binding = 0) uniform sampler2D diffuseMap;
layout(binding = 1) uniform sampler2D specularMap;
layout(binding = 2) uniform sampler2D shadowMap; // the sun's depth, rendered from its point of view

uniform bool shadowsEnabled;
uniform float shadowBiasMin;
uniform float shadowBiasMax;
uniform bool pcf;

#define MAX_POINT_LIGHTS 4 // must match kMaxPointLights in light.h

struct DirLight {
    bool enabled;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    bool enabled;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cutOff;      // cos of the inner angle
    float outerCutOff; // cos of the outer angle
};

uniform Material material;
uniform DirLight dirLight;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int numPointLights;
uniform SpotLight spotLight;
uniform vec3 viewPos;
uniform bool blinn;

// Per-pixel surface values, set once in main() and shared by every light.
vec3 n;         // normal
vec3 v;         // towards the camera
vec3 albedo;    // diffuse color
vec3 specColor; // specular color

// Diffuse + specular from one light arriving along direction l (towards the light).
vec3 shade(vec3 l, vec3 lightDiffuse, vec3 lightSpecular) {
    float diff = max(dot(n, l), 0.0);
    float spec = 0.0;

    // Only surfaces facing the light get a highlight. Blinn's halfway vector can still line up with the
    // normal on faces turned away, which shows up as sparkles on back-facing edges.
    if (diff > 0.0) {
        if (blinn) {
            // Blinn-Phong: normal vs halfway vector. Needs ~2-4x Phong's shininess for the same highlight size.
            spec = pow(max(dot(n, normalize(l + v)), 0.0), material.shininess);
        } else {
            // Phong: view vs mirrored light direction. Cut off with a hard edge past 90 degrees.
            spec = pow(max(dot(v, reflect(-l, n)), 0.0), material.shininess);
        }
    }

    return lightDiffuse * diff * albedo + lightSpecular * spec * specColor;
}

float attenuation(vec3 lightPos, float constant, float linear, float quadratic) {
    float d = length(lightPos - vPos);
    return 1.0 / (constant + linear * d + quadratic * d * d);
}

// How much this pixel is hidden from the sun: 0 = lit, 1 = fully in shadow.
float sunShadow(vec3 l) {
    // Clip space -> [0,1]: xy is where to look in the shadow map, z is this pixel's depth as the sun sees it.
    vec3 p = vLightSpacePos.xyz / vLightSpacePos.w * 0.5 + 0.5;

    if (p.z > 1.0)
        return 0.0; // beyond the light box's far plane

    // Each shadow-map texel covers a patch of surface; on a slope, part of that patch is "behind" the stored
    // depth and shadows itself (striped "shadow acne"). Bias pushes the comparison back, more for surfaces
    // at a grazing angle to the light. Too much and shadows detach from their objects ("peter-panning").
    float bias = max(shadowBiasMax * (1.0 - dot(n, l)), shadowBiasMin);

    if (!pcf)
        return p.z - bias > texture(shadowMap, p.xy).r ? 1.0 : 0.0;

    // Percentage-closer filtering: the fraction of the 3x3 neighboring texels that block the light,
    // which turns the hard staircase edge into a soft one.
    vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
    float shadow = 0.0;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float closest = texture(shadowMap, p.xy + vec2(x, y) * texel).r;
            shadow += p.z - bias > closest ? 1.0 : 0.0;
        }
    }

    return shadow / 9.0;
}

vec3 calcDirLight(DirLight light) {
    // Parallel rays: the same direction for every pixel. The light points at the scene, we need the way back.
    vec3 l = normalize(-light.direction);
    float shadow = shadowsEnabled ? sunShadow(l) : 0.0;
    return light.ambient * albedo + (1.0 - shadow) * shade(l, light.diffuse, light.specular);
}

vec3 calcPointLight(PointLight light) {
    vec3 l = normalize(light.position - vPos);
    float att = attenuation(light.position, light.constant, light.linear, light.quadratic);
    return att * (light.ambient * albedo + shade(l, light.diffuse, light.specular));
}

vec3 calcSpotLight(SpotLight light) {
    vec3 l = normalize(light.position - vPos);
    float att = attenuation(light.position, light.constant, light.linear, light.quadratic);

    // Cone with a soft edge: 1 inside the inner angle, 0 outside the outer, linear fade between.
    // Compared as cosines (dot gives them directly); cos grows as the angle shrinks, so cutOff > outerCutOff.
    float theta = dot(l, normalize(-light.direction));
    float intensity = clamp((theta - light.outerCutOff) / (light.cutOff - light.outerCutOff), 0.0, 1.0);

    // Ambient ignores the cone, so outside it isn't pitch black.
    return att * (light.ambient * albedo + intensity * shade(l, light.diffuse, light.specular));
}

void main() {
    n = normalize(vNormal);
    v = normalize(viewPos - vPos);
    albedo = texture(diffuseMap, vUV).rgb * material.diffuse;
    specColor = texture(specularMap, vUV).rgb * material.specular;

    // Light is additive: each light's contribution is independent, the total is their sum.
    vec3 result = vec3(0.0);

    if (dirLight.enabled)
        result += calcDirLight(dirLight);

    for (int i = 0; i < numPointLights; i++)
        result += calcPointLight(pointLights[i]);

    if (spotLight.enabled)
        result += calcSpotLight(spotLight);

    color = vec4(result, 1.0);
}
