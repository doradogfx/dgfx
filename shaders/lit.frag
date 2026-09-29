#version 460 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;

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

vec3 calcDirLight(DirLight light) {
    // Parallel rays: the same direction for every pixel. The light points at the scene, we need the way back.
    vec3 l = normalize(-light.direction);
    return light.ambient * albedo + shade(l, light.diffuse, light.specular);
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
