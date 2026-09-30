#version 460 core

layout(location = 0) in vec3 pos;

out vec3 vDir;

uniform mat4 view;
uniform mat4 projection;

void main() {
    // The cube's vertex position doubles as the direction to sample the cubemap with.
    vDir = pos;

    // mat3(view) keeps only the camera's rotation: the sky turns with you but never gets closer.
    vec4 clip = projection * mat4(mat3(view)) * vec4(pos, 1.0);

    // z = w makes depth exactly 1.0 after the perspective divide: the sky sits at the far plane,
    // behind everything, and is only drawn where nothing else covered the pixel.
    gl_Position = clip.xyww;
}
