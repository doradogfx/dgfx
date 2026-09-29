#version 460 core

in vec3 vPos;
in vec3 vNormal;

out vec4 color;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position; // world space
    vec3 ambient;  // light that reaches everywhere, even faces turned away
    vec3 diffuse;  // the light's main color and brightness
    vec3 specular; // brightness of the highlights it causes
};

uniform Material material;
uniform Light light;
uniform vec3 viewPos; // camera position, world space
uniform bool blinn;   // true = Blinn-Phong specular, false = classic Phong

void main() {
    // Interpolation between vertices shortens normals, so make it unit length again.
    vec3 n = normalize(vNormal);
    // Direction from this fragment towards the light, and towards the camera.
    vec3 l = normalize(light.position - vPos);
    vec3 v = normalize(viewPos - vPos);

    // Ambient: a constant floor, a cheap stand-in for light bouncing around the scene.
    vec3 ambient = light.ambient * material.ambient;

    // Diffuse: brightest when the surface faces the light, fading to 0 at 90 degrees.
    // dot of two unit vectors = cos of the angle between them; max() drops faces turned away (negative).
    vec3 diffuse = light.diffuse * max(dot(n, l), 0.0) * material.diffuse;

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

    vec3 specular = light.specular * spec * material.specular;

    // Remove terms from this sum to see what each one contributes.
    color = vec4(ambient + diffuse + specular, 1.0);
}
