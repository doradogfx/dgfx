#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

Renderer::Renderer(int width, int height)
    : lit(SHADER_DIR "lit.vert", SHADER_DIR "lit.frag"),
      lamp(SHADER_DIR "lit.vert", SHADER_DIR "light.frag"),
      post(SHADER_DIR "post.vert", SHADER_DIR "post.frag"),
      sceneTarget(width, height) {
    // The post pass's full-screen triangle comes from gl_VertexID alone, but core profile still requires
    // a VAO to be bound for any draw, so an empty one.
    glGenVertexArrays(1, &emptyVao);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &emptyVao);
}

void Renderer::render(Scene& scene, const Camera& camera, int width, int height, float time) {
    // Scene pass, into the off-screen target.
    sceneTarget.resize(width, height);
    sceneTarget.bind();
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glEnable(GL_DEPTH_TEST);

    // The scene is computed in linear space; the GPU encodes to sRGB when writing each pixel (and the clear).
    glEnable(GL_FRAMEBUFFER_SRGB);

    glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
    const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
    const glm::mat4 view = camera.view();

    // The panel edits base positions; orbiting rotates copies of them around the Y axis.
    PointLight worldPoints[kMaxPointLights];
    const glm::mat4 orbit = glm::rotate(glm::mat4(1.0f), scene.orbitLights ? time * 0.5f : 0.0f, Camera::worldUp);

    for (int i = 0; i < kMaxPointLights; i++) {
        worldPoints[i] = scene.points[i];
        worldPoints[i].position = glm::vec3(orbit * glm::vec4(scene.points[i].position, 1.0f));
    }

    scene.flashlight.position = camera.position;
    scene.flashlight.direction = camera.front();

    lit.use();
    lit.setMat4("projection", projection);
    lit.setMat4("view", view);
    lit.setVec3("viewPos", camera.position);
    lit.setBool("blinn", blinn);
    setLights(lit, scene.sun, worldPoints, scene.flashlight);

    for (const Object& obj : scene.objects) {
        // Right to left: scale, then rotate around the object's center, then move it into place.
        glm::mat4 model = glm::translate(glm::mat4(1.0f), obj.position);
        model = glm::rotate(model, glm::radians(obj.yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, obj.scale);

        // Normals can't just use the model matrix: a non-uniform scale (like the flattened floor) would
        // tilt them so they no longer point straight out of the surface. The inverse transpose undoes the
        // scale's effect on direction while keeping rotation. mat3 drops translation, directions don't move.
        const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

        lit.setMat4("model", model);
        lit.setMat3("normalMatrix", normalMatrix);
        // Maps go to the units the shader's samplers read (layout binding 0 and 1).
        (obj.material.diffuseMap ? obj.material.diffuseMap : &scene.white)->bind(0);
        (obj.material.specularMap ? obj.material.specularMap : &scene.white)->bind(1);
        lit.setVec3("material.diffuse", obj.material.diffuse);
        lit.setVec3("material.specular", obj.material.specular);
        lit.setFloat("material.shininess", obj.material.shininess);

        obj.mesh->draw();
    }

    // A small sphere per enabled point light, in its color.
    lamp.use();
    lamp.setMat4("projection", projection);
    lamp.setMat4("view", view);

    for (const PointLight& p : worldPoints) {
        if (!p.enabled)
            continue;

        lamp.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), p.position), glm::vec3(0.15f)));
        lamp.setVec3("lightColor", p.diffuse);
        scene.sphere.draw();
    }

    // Post pass: the scene texture through the post shader onto the window, one full-screen triangle.
    // sRGB stays on, so the window gets the final encoding.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);

    post.use();
    post.setInt("effect", postEffect);
    post.setFloat("gamma", gamma);
    sceneTarget.bindColor(0);
    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Leave sRGB off for whatever draws next (the UI is already sRGB).
    glDisable(GL_FRAMEBUFFER_SRGB);
}
