#include "renderer/renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

Renderer::Renderer(int width, int height)
    : lit(SHADER_DIR "lit.vert", SHADER_DIR "lit.frag"),
      lamp(SHADER_DIR "lit.vert", SHADER_DIR "light.frag"),
      post(SHADER_DIR "post.vert", SHADER_DIR "post.frag"),
      depth(SHADER_DIR "shadow.vert", SHADER_DIR "shadow.frag"),
      skybox(SHADER_DIR "skybox.vert", SHADER_DIR "skybox.frag"),
      sceneTarget(width, height),
      shadowMap(shadowResolution) {
    // The post pass's full-screen triangle comes from gl_VertexID alone, but core profile still requires
    // a VAO to be bound for any draw, so an empty one.
    glGenVertexArrays(1, &emptyVao);

    // Filter across cubemap face edges, otherwise seams show along the sky cube's edges.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &emptyVao);
}

// A model's entity transform places its base (bottom center), so move the base to the origin first.
static glm::mat4 modelMatrix(const Scene& scene, entt::entity entity, const Model& model) {
    return glm::translate(scene.worldMatrix(entity), -model.base());
}

// The sun as a camera: an orthographic box (parallel rays, no perspective) around the scene, looking along
// the sun's direction. Fixed around the origin and sized for the 10x10 floor.
static glm::mat4 sunLightSpace(const DirLight& sun) {
    const glm::vec3 dir = glm::normalize(sun.direction);
    // lookAt can't build a view when "up" is parallel to the view direction, i.e. a sun straight overhead.
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : Camera::worldUp;
    const glm::mat4 view = glm::lookAt(-dir * 10.0f, glm::vec3(0.0f), up);
    const glm::mat4 projection = glm::ortho(-8.0f, 8.0f, -8.0f, 8.0f, 0.1f, 20.0f);
    return projection * view;
}

// Skip triangles facing away (GL_BACK) or towards the camera (GL_FRONT). Front = counter-clockwise on screen.
static void setCulling(bool enabled, GLenum face) {
    if (!enabled) {
        glDisable(GL_CULL_FACE);
        return;
    }

    glEnable(GL_CULL_FACE);
    glCullFace(face);
}

// One mesh with the lit shader, which must already be in use with the per-frame uniforms set.
void Renderer::drawLit(const Scene& scene, const Mesh& mesh, const Material& material, const glm::mat4& model) {
    // Normals can't just use the model matrix: a non-uniform scale (like the flattened floor) would
    // tilt them so they no longer point straight out of the surface. The inverse transpose undoes the
    // scale's effect on direction while keeping rotation. mat3 drops translation, directions don't move.
    const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));

    lit.setMat4("model", model);
    lit.setMat3("normalMatrix", normalMatrix);
    // Maps go to the units the shader's samplers read (layout binding 0 and 1).
    (material.diffuseMap ? material.diffuseMap : &scene.white)->bind(0);
    (material.specularMap ? material.specularMap : &scene.white)->bind(1);
    lit.setVec3("material.diffuse", material.diffuse);
    lit.setVec3("material.specular", material.specular);
    lit.setFloat("material.shininess", material.shininess);
    lit.setFloat("material.reflectivity", material.reflectivity);
    lit.setFloat("material.refractivity", material.refractivity);
    lit.setFloat("material.ior", material.ior);

    setCulling(faceCulling && !material.doubleSided, GL_BACK);
    mesh.draw();
}

void Renderer::render(Scene& scene, const Camera& camera, int width, int height) {
    entt::registry& registry = scene.registry;

    // Gather the lights from their entities. The shader takes one sun, up to kMaxPointLights point lights
    // and one spot; extra ones are ignored. A missing light is sent disabled.
    DirLight sun;
    sun.enabled = false;

    for (auto [entity, light] : registry.view<DirLight>().each()) {
        if (light.enabled) {
            sun = light;
            break;
        }
    }

    std::vector<PointLight> points;

    for (auto [entity, light] : registry.view<PointLight>().each()) {
        if (light.enabled && points.size() < kMaxPointLights) {
            points.push_back(light);
            points.back().position = glm::vec3(scene.worldMatrix(entity)[3]); // translation column
        }
    }

    // The flashlight follows the camera.
    SpotLight spot;

    for (auto [entity, light] : registry.view<SpotLight>().each()) {
        spot = light;
        spot.position = camera.position;
        spot.direction = camera.front();
        break;
    }

    const glm::mat4 lightSpace = sunLightSpace(sun);
    const bool castShadows = shadows && sun.enabled;

    // Shadow pass: the scene's depth as the sun sees it. Lamps don't cast shadows.
    if (castShadows) {
        shadowMap.resize(shadowResolution);
        shadowMap.bind();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
        setCulling(faceCulling || shadowCullFront, shadowCullFront ? GL_FRONT : GL_BACK);
        glClear(GL_DEPTH_BUFFER_BIT);

        depth.use();
        depth.setMat4("lightSpace", lightSpace);

        for (auto [entity, meshRenderer] : registry.view<MeshRenderer>().each()) {
            depth.setMat4("model", scene.worldMatrix(entity));
            meshRenderer.mesh->draw();
        }

        for (auto [entity, modelRenderer] : registry.view<ModelRenderer>().each()) {
            depth.setMat4("model", modelMatrix(scene, entity, *modelRenderer.model));

            for (const Model::Part& part : modelRenderer.model->parts) {
                // Thin double-sided surfaces must cast shadows whichever side faces the sun.
                setCulling((faceCulling || shadowCullFront) && !part.material.doubleSided, shadowCullFront ? GL_FRONT : GL_BACK);
                part.mesh.draw();
            }
        }
    }

    // Scene pass: into the off-screen target.
    sceneTarget.resize(width, height);
    sceneTarget.bind();
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glEnable(GL_DEPTH_TEST);
    setCulling(faceCulling, GL_BACK);

    // The scene is computed in linear space; the GPU encodes to sRGB when writing each pixel (and the clear).
    glEnable(GL_FRAMEBUFFER_SRGB);

    glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
    const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, 0.1f, 100.0f);
    const glm::mat4 view = camera.view();

    lit.use();
    lit.setMat4("projection", projection);
    lit.setMat4("view", view);
    lit.setVec3("viewPos", camera.position);
    lit.setBool("blinn", blinn);
    setLights(lit, sun, points, spot);

    lit.setMat4("lightSpace", lightSpace);
    lit.setBool("shadowsEnabled", castShadows);
    lit.setFloat("shadowBiasMin", shadowBiasMin);
    lit.setFloat("shadowBiasMax", shadowBiasMax);
    lit.setBool("pcf", pcf);
    shadowMap.bindDepth(2);

    lit.setBool("reflections", reflections);
    lit.setBool("fresnel", fresnel);
    scene.sky.bind(3);

    for (auto [entity, meshRenderer] : registry.view<MeshRenderer>().each())
        drawLit(scene, *meshRenderer.mesh, meshRenderer.material, scene.worldMatrix(entity));

    for (auto [entity, modelRenderer] : registry.view<ModelRenderer>().each()) {
        const glm::mat4 model = modelMatrix(scene, entity, *modelRenderer.model);

        for (const Model::Part& part : modelRenderer.model->parts)
            drawLit(scene, part.mesh, part.material, model);
    }

    setCulling(faceCulling, GL_BACK);

    // A small sphere per enabled point light, in its color.
    lamp.use();
    lamp.setMat4("projection", projection);
    lamp.setMat4("view", view);

    for (const PointLight& p : points) {
        lamp.setMat4("model", glm::scale(glm::translate(glm::mat4(1.0f), p.position), glm::vec3(0.15f)));
        lamp.setVec3("lightColor", p.diffuse);
        scene.sphere.draw();
    }

    // Sky last: its depth is 1.0, so the depth test skips every pixel an object already covered.
    // LEQUAL because it has to pass against the cleared depth, which is also 1.0.
    // Culling off because we're inside the cube, looking at its back faces.
    if (showSkybox) {
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        skybox.use();
        skybox.setMat4("projection", projection);
        skybox.setMat4("view", view);
        scene.sky.bind(0);
        scene.cube.draw();

        glDepthFunc(GL_LESS);
    }

    // Post pass: the scene texture through the post shader onto the window, one full-screen triangle.
    // sRGB stays on, so the window gets the final encoding.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    post.use();
    post.setInt("effect", postEffect);
    post.setFloat("gamma", gamma);
    sceneTarget.bindColor(0);
    glBindVertexArray(emptyVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Leave sRGB off for whatever draws next (the UI is already sRGB).
    glDisable(GL_FRAMEBUFFER_SRGB);
}
