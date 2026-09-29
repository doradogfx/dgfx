#version 460 core

in vec3 vPos;
in vec3 vNormal;
in vec2 vUV;

out vec4 color;

// Colors are tints, multiplied with what the maps below return at this pixel.
// Textured materials use white tints; solid-color ones get a 1x1 white map, leaving just the tint.
struct Material {
    vec3 diffuse;    // surface color (also used for ambient)
    vec3 specular;   // highlight color
    float shininess;
};

// Lighting maps: material colors that vary per pixel. They live outside the struct because
// layout(binding) isn't allowed on struct members, and binding = N saves setting the unit from C++.
layout(binding = 0) uniform sampler2D diffuseMap;  // the surface's color, like a regular texture
layout(binding = 1) uniform sampler2D specularMap; // where it shines: black = no highlight, white = full

// Light type values, matching LightType in main.cpp.
const int DIRECTIONAL = 0; // the sun: parallel rays, no position, no falloff
const int POINT = 1;       // a bulb: shines in all directions from a position, fades with distance
const int SPOT = 2;        // a flashlight: a point light restricted to a cone

struct Light {
    int type;
    vec3 position;     // world space. Point and spot
    vec3 direction;    // where the light points, world space. Directional and spot
    vec3 ambient;      // light that reaches everywhere, even faces turned away
    vec3 diffuse;      // the light's main color and brightness
    vec3 specular;     // brightness of the highlights it causes
    float constant;    // attenuation 1 / (constant + linear * d + quadratic * d^2), d = distance to the light.
    float linear;      // Point and spot
    float quadratic;
    float cutOff;      // spot: cos of the inner cone angle, full brightness inside it
    float outerCutOff; // spot: cos of the outer cone angle, dark outside it, fading in between
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos; // camera position, world space
uniform bool blinn;   // true = Blinn-Phong specular, false = classic Phong

void main() {
    // Interpolation between vertices shortens normals, so make it unit length again.
    vec3 n = normalize(vNormal);
    // Direction from this fragment towards the camera.
    vec3 v = normalize(viewPos - vPos);

    // Direction from this fragment towards the light. A directional light is so far away (the sun) that its
    // rays are parallel: the same direction for every pixel. That's the whole difference to a point light.
    vec3 l;
    float attenuation = 1.0;

    if (light.type == DIRECTIONAL) {
        l = normalize(-light.direction); // the light points *at* the scene; we want the way back towards it
    } else {
        l = normalize(light.position - vPos);

        // Farther from the light = dimmer. The quadratic term is what makes it fall off like real light;
        // constant (1) keeps it from blowing up to infinity right next to the light, linear shapes the middle.
        float d = length(light.position - vPos);
        attenuation = 1.0 / (light.constant + light.linear * d + light.quadratic * d * d);
    }

    // Spot cone: 1 inside the inner angle, 0 outside the outer one, a smooth fade between them (the soft edge).
    // Everything is compared as cosines: dot() of unit vectors gives the cosine directly, no acos per pixel.
    // A cosine grows as the angle shrinks, which is why inner (cutOff) is the *bigger* value.
    float intensity = 1.0;

    if (light.type == SPOT) {
        float theta = dot(l, normalize(-light.direction)); // cos of angle between this pixel and the spot's axis
        intensity = clamp((theta - light.outerCutOff) / (light.cutOff - light.outerCutOff), 0.0, 1.0);
    }

    // This pixel's material colors. A greyscale specular map works directly: grey (r = g = b) scales all channels equally.
    vec3 albedo = texture(diffuseMap, vUV).rgb * material.diffuse;
    vec3 specColor = texture(specularMap, vUV).rgb * material.specular;

    // Ambient: a constant floor, a cheap stand-in for light bouncing around the scene.
    vec3 ambient = light.ambient * albedo;

    // Diffuse: brightest when the surface faces the light, fading to 0 at 90 degrees.
    // dot of two unit vectors = cos of the angle between them; max() drops faces turned away (negative).
    vec3 diffuse = light.diffuse * max(dot(n, l), 0.0) * albedo;

    // Specular: the highlight, seen where light bounces off the surface towards the camera.
    float spec;

    if (blinn) {
        // Blinn-Phong: how close the normal is to the halfway vector between light and view directions.
        // That angle never passes 90 degrees while the surface faces the light, so the highlight fades smoothly.
        // Its angle is smaller than Phong's, so it needs roughly 2-4x the shininess for a same-sized highlight.
        vec3 h = normalize(l + v);
        spec = pow(max(dot(n, h), 0.0), material.shininess);
    } else {
        // Phong: how close the view direction is to the mirrored light direction.
        // reflect() wants the incoming direction (light -> surface), hence -l. Past 90 degrees between v and r
        // the dot goes negative and max() clamps it to 0, which cuts the highlight off with a hard edge;
        // visible on the floor at low shininess.
        vec3 r = reflect(-l, n);
        spec = pow(max(dot(v, r), 0.0), material.shininess);
    }

    vec3 specular = light.specular * spec * specColor;

    // Distance dims all terms. The spot cone only limits diffuse and specular: ambient stays, so the area
    // outside the cone isn't pitch black.
    ambient *= attenuation;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    // Remove terms from this sum to see what each one contributes.
    color = vec4(ambient + diffuse + specular, 1.0);
}
