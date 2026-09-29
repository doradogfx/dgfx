#version 460 core

in vec3 vPos;
in vec3 vNormal;

out vec4 color;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;          // world space
uniform vec3 viewPos;           // camera position, world space
uniform float ambientStrength;  // light that reaches everywhere, even faces turned away
uniform float specularStrength; // how bright the shiny highlight is
uniform float shininess;        // how tight the highlight is: higher = smaller, sharper spot

// Phong lighting: the final color is the sum of three terms, each tinted by the light and the object.
void main() {
    // Interpolation between vertices shortens normals, so make it unit length again.
    vec3 n = normalize(vNormal);
    // Direction from this fragment towards the light.
    vec3 l = normalize(lightPos - vPos);

    // Ambient: a constant floor, a cheap stand-in for light bouncing around the scene.
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse: brightest when the surface faces the light, fading to 0 at 90 degrees.
    // dot of two unit vectors = cos of the angle between them; max() drops faces turned away (negative).
    vec3 diffuse = max(dot(n, l), 0.0) * lightColor;

    // Specular: the highlight, seen where light bounces off the surface straight into the camera.
    // reflect() wants the incoming direction (light -> surface), hence -l.
    vec3 r = reflect(-l, n);
    vec3 v = normalize(viewPos - vPos);
    vec3 specular = specularStrength * pow(max(dot(v, r), 0.0), shininess) * lightColor;

    // Remove terms from this sum to see what each one contributes.
    color = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}
