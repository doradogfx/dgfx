#include "renderer/renderer.h"

#include "core/paths.h"

#include <glm/gtc/matrix_transform.hpp>

#include <vector>

static std::string shaderPath(const char* file) {
    return assetRoot() + "shaders/" + file;
}

Renderer::Renderer(int width, int height)
    : lit(shaderPath("lit.vert"), shaderPath("lit.frag")),
      lamp(shaderPath("lit.vert"), shaderPath("light.frag")),
      post(shaderPath("post.vert"), shaderPath("post.frag")),
      depth(shaderPath("shadow.vert"), shaderPath("shadow.frag")),
      skybox(shaderPath("skybox.vert"), shaderPath("skybox.frag")),
      cube(makeCube()),
      sphere(makeSphere()),
      white(glm::vec3(1.0f)),
      sceneTarget(width, height),
      shadowMap(shadowResolution) {
    // The post pass's full-screen triangle comes from gl_VertexID alone, but core profile still requires
    // a VAO to be bound for any draw, so an empty one.
    glGenVertexArrays(1, &emptyVao);
    glCreateBuffers(1, &instanceBuffer);

    // Filter across cubemap face edges, otherwise seams show along the sky cube's edges.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Renderer::~Renderer() {
    glDeleteBuffers(1, &instanceBuffer);
    glDeleteVertexArrays(1, &emptyVao);
}

// A model's entity transform places its base (bottom center), so move the base to the origin first.
static glm::mat4 modelMatrix(const Scene& scene, entt::entity entity, const Model& model) {
    return glm::translate(scene.worldMatrix(entity), -model.base());
}

// The sun as a camera: an orthographic box (parallel rays, no perspective) around a sphere, looking along the
// sun's direction. Only what's inside the box casts or receives shadows. A sphere looks the same from every
// direction, so the box holds all of it at any sun angle. The box does not move with the camera, so the shadow
// edges do not shimmer.
static glm::mat4 sunLightSpace(glm::vec3 direction, glm::vec3 center, float radius) {
    const glm::vec3 dir = glm::normalize(direction);
    // lookAt can't build a view when "up" is parallel to the view direction, i.e. a sun straight overhead.
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : Camera::worldUp;
    const glm::mat4 view = glm::lookAt(center - dir * radius, center, up);
    const glm::mat4 projection = glm::ortho(-radius, radius, -radius, radius, 0.0f, 2.0f * radius);
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

// Groups the objects by mesh and material, then puts the data of all copies into instanceData, batch after batch.
void Renderer::buildBatches(const Scene& scene) {
    batches.clear();

    // ponytail: a linear search for the batch of each object. Fine for a few batches, use a hash map for many.
    auto add = [this](const Mesh* mesh, const Material& material, const glm::mat4& model) {
        for (Batch& batch : batches) {
            if (batch.mesh == mesh && batch.material == material) {
                batch.models.push_back(model);
                return;
            }
        }

        batches.push_back({mesh, material, 0, {model}});
    };

    for (auto [entity, meshRenderer] : scene.registry.view<const MeshRenderer>().each())
        add(meshRenderer.mesh, meshRenderer.material, scene.worldMatrix(entity));

    for (auto [entity, modelRenderer] : scene.registry.view<const ModelRenderer>().each()) {
        const glm::mat4 model = modelMatrix(scene, entity, *modelRenderer.model);

        for (const Model::Part& part : modelRenderer.model->parts)
            add(&part.mesh, part.material, model);
    }

    instanceData.clear();

    for (Batch& batch : batches) {
        batch.first = static_cast<GLuint>(instanceData.size());

        // Normals can't just use the model matrix: a non-uniform scale (like a flat rock) would tilt them so
        // they no longer point straight out of the surface. The inverse transpose undoes the scale's effect on
        // direction while keeping rotation. mat3 drops translation, directions don't move.
        for (const glm::mat4& model : batch.models)
            instanceData.push_back({model, glm::mat4(glm::transpose(glm::inverse(glm::mat3(model))))});
    }
}

// One batch with the lit shader, which must already be in use with the per-frame uniforms set.
void Renderer::drawBatch(const Batch& batch) {
    const Material& material = batch.material;

    // Maps go to the units the shader's samplers read (layout binding 0 and 1).
    (material.diffuseMap ? material.diffuseMap : &white)->bind(0);
    (material.specularMap ? material.specularMap : &white)->bind(1);
    lit.setVec3("material.diffuse", material.diffuse);
    lit.setVec3("material.specular", material.specular);
    lit.setFloat("material.shininess", material.shininess);
    lit.setFloat("material.reflectivity", material.reflectivity);
    lit.setFloat("material.refractivity", material.refractivity);
    lit.setFloat("material.ior", material.ior);

    setCulling(faceCulling && !material.doubleSided, GL_BACK);
    batch.mesh->drawInstanced(static_cast<GLsizei>(batch.models.size()), batch.first);
    drawCalls++;
    instances += static_cast<int>(batch.models.size());
}

void Renderer::render(Scene& scene, const Camera& camera, int width, int height) {
    entt::registry& registry = scene.registry;
    drawCalls = 0;
    instances = 0;

    // Gather the lights from their entities. The shader takes one sun, up to kMaxPointLights point lights
    // and one spot; extra ones are ignored. A missing light is sent disabled.
    // Position and direction come from each light's entity transform.
    WorldDirLight sun;
    sun.light.enabled = false;

    for (auto [entity, light] : registry.view<DirLight>().each()) {
        if (light.enabled) {
            sun = {light, scene.forward(entity)};
            break;
        }
    }

    std::vector<WorldPointLight> points;

    for (auto [entity, light] : registry.view<PointLight>().each()) {
        if (light.enabled && points.size() < kMaxPointLights)
            points.push_back({light, scene.position(entity)});
    }

    WorldSpotLight spot;

    for (auto [entity, light] : registry.view<SpotLight>().each()) {
        spot = {light, scene.position(entity), scene.forward(entity)};
        break;
    }

    const glm::mat4 lightSpace = sunLightSpace(sun.direction, shadowCenter, shadowRadius);
    const bool castShadows = shadows && sun.light.enabled;

    // The data of every copy goes to the GPU once, in one buffer that both passes read. The point light lamps
    // come last, one copy each.
    buildBatches(scene);
    const GLuint firstLamp = static_cast<GLuint>(instanceData.size());

    for (const WorldPointLight& p : points) {
        const glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), p.position), glm::vec3(0.15f));
        instanceData.push_back({model, glm::mat4(1.0f)});
    }

    glNamedBufferData(instanceBuffer, static_cast<GLsizeiptr>(instanceData.size() * sizeof(Instance)), instanceData.data(), GL_STREAM_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, instanceBuffer);

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

        for (const Batch& batch : batches) {
            // Thin double-sided surfaces must cast shadows whichever side faces the sun.
            setCulling((faceCulling || shadowCullFront) && !batch.material.doubleSided, shadowCullFront ? GL_FRONT : GL_BACK);
            batch.mesh->drawInstanced(static_cast<GLsizei>(batch.models.size()), batch.first);
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

    lit.setBool("reflections", reflections && scene.sky);
    lit.setBool("fresnel", fresnel);
    if (scene.sky)
        scene.sky->bind(3);

    for (const Batch& batch : batches)
        drawBatch(batch);

    setCulling(faceCulling, GL_BACK);

    // A small sphere per enabled point light, in its color.
    lamp.use();
    lamp.setMat4("projection", projection);
    lamp.setMat4("view", view);

    for (size_t i = 0; i < points.size(); i++) {
        lamp.setVec3("lightColor", points[i].light.diffuse);
        sphere.drawInstanced(1, firstLamp + static_cast<GLuint>(i));
        drawCalls++;
        instances++;
    }

    // Sky last: its depth is 1.0, so the depth test skips every pixel an object already covered.
    // LEQUAL because it has to pass against the cleared depth, which is also 1.0.
    // Culling off because we're inside the cube, looking at its back faces.
    if (showSkybox && scene.sky) {
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        skybox.use();
        skybox.setMat4("projection", projection);
        skybox.setMat4("view", view);
        scene.sky->bind(0);
        cube.draw();

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
