#include "renderer/renderer.h"

#include "core/paths.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdio>
#include <vector>

static std::string shaderPath(const char* file) {
    return assetRoot() + "shaders/" + file;
}

Renderer::Renderer()
    : lit(shaderPath("lit.vert"), shaderPath("lit.frag")),
      lamp(shaderPath("lit.vert"), shaderPath("light.frag")),
      post(shaderPath("post.vert"), shaderPath("post.frag")),
      depth(shaderPath("shadow.vert"), shaderPath("shadow.frag")),
      skybox(shaderPath("skybox.vert"), shaderPath("skybox.frag")),
      cube(makeCube()),
      sphere(makeSphere()),
      white(glm::vec3(1.0f)),
      msaaTarget(1, 1, msaaSamples), // render() resizes both to the window on the first frame
      sceneTarget(1, 1),
      shadowMap(shadowResolution) {
    // The post pass's full-screen triangle comes from gl_VertexID alone, but core profile still requires
    // a VAO to be bound for any draw, so an empty one.
    glGenVertexArrays(1, &emptyVao);
    glCreateBuffers(1, &instanceBuffer);
    glCreateBuffers(1, &lightBuffer);
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    // Filter across cubemap face edges, otherwise seams show along the sky cube's edges.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

Renderer::~Renderer() {
    glDeleteBuffers(1, &lightBuffer);
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
    const glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : kWorldUp;
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

void Renderer::render(Scene& scene, int width, int height) {
    entt::registry& registry = scene.registry;
    drawCalls = 0;
    instances = 0;

    const entt::entity cameraEntity = scene.camera();

    if (cameraEntity == entt::null) {
        static bool reported = false;

        if (!reported)
            std::fprintf(stderr, "The scene has no Camera entity, nothing is drawn\n");

        reported = true;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    const Camera& camera = registry.get<Camera>(cameraEntity);
    const glm::vec3 cameraPosition = scene.position(cameraEntity);

    // Gather the lights from their entities: one sun (uniforms), and any number of point and spot lights (a
    // storage buffer). A missing sun is sent disabled. Position and direction come from each light's entity transform.
    WorldDirLight sun;
    sun.light.enabled = false;

    const Sky* sky = nullptr; // the procedural sky, on the sun's entity

    for (auto [entity, light] : registry.view<DirLight>().each()) {
        if (light.enabled) {
            sun = {light, scene.forward(entity)};
            sky = registry.try_get<Sky>(entity);
            break;
        }
    }

    std::vector<WorldPointLight> points; // also kept for the lamps
    lightData.clear();

    for (auto [entity, light] : registry.view<PointLight>().each()) {
        if (light.enabled) {
            points.push_back({light, scene.position(entity)});
            lightData.push_back(toGpu(points.back()));
        }
    }

    for (auto [entity, light] : registry.view<SpotLight>().each()) {
        if (light.enabled)
            lightData.push_back(toGpu(WorldSpotLight{light, scene.position(entity), scene.forward(entity)}));
    }

    // At least one element: a buffer with no storage can't be bound.
    const GLsizeiptr lightBytes = static_cast<GLsizeiptr>(std::max<size_t>(lightData.size(), 1) * sizeof(GpuLight));
    glNamedBufferData(lightBuffer, lightBytes, nullptr, GL_STREAM_DRAW);
    glNamedBufferSubData(lightBuffer, 0, static_cast<GLsizeiptr>(lightData.size() * sizeof(GpuLight)), lightData.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, lightBuffer);

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

    profiler.beginFrame();

    // The procedural sky: draw it again if the sun moved, then let it light the scene. The sun's light takes the
    // color of the sunlight that reaches the ground (white at noon, orange low, black below the horizon), at the
    // strength the Sky asks for. The sky's average color becomes the ambient.
    const glm::vec3 toSun = -glm::normalize(sun.direction);

    if (sky) {
        proceduralSky.update(*sky, toSun, cube);

        // Night: the moon takes over the light, so the scene stays readable. It is opposite the sun, raised to at
        // least 45 degrees so its light does not only graze the ground. It fades in over the first 10 degrees
        // below the horizon (sin 10 = 0.17), where the sunlight is already gone.
        const glm::vec3 moonColor = glm::vec3(0.6f, 0.75f, 1.0f) * sky->moonLight;
        const float night = glm::smoothstep(0.0f, 0.17f, -toSun.y);

        if (night > 0.0f) {
            // Raised only when it is lower than 45 degrees. Its horizontal direction is then at least 0.7 long, so
            // it never flips: a moon straight up (sun straight down) needs no horizontal direction at all.
            const float sin45 = 0.70710678f;
            glm::vec3 toMoon = -toSun;

            if (toMoon.y < sin45) {
                const glm::vec2 away = glm::normalize(glm::vec2(toMoon.x, toMoon.z)) * sin45;
                toMoon = glm::vec3(away.x, sin45, away.y);
            }

            sun.direction = -toMoon;
            sun.light.diffuse = sun.light.specular = moonColor * night;
        } else {
            sun.light.diffuse = sun.light.specular = ProceduralSky::sunColor(*sky, toSun) * sky->sunLight;
        }

        // A dim blue floor under the sky's light, day and night: at dusk the sunlight is weak and the sky is dark,
        // and it keeps the sides away from the moon readable.
        sun.light.ambient = proceduralSky.ambient() * sky->ambientStrength + moonColor * 0.5f;
    }

    profiler.mark("Sky"); // also without a sky, so the list of passes stays the same

    // At night the sun's direction is the moon's, so the moon casts the shadows.
    const glm::mat4 lightSpace = sunLightSpace(sun.direction, shadowCenter, shadowRadius);
    const bool castShadows = shadows && sun.light.enabled;

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

    profiler.mark("Shadow"); // also when shadows are off, so the list of passes stays the same

    // Scene pass: into the off-screen target, or with MSAA into the multisample one that is resolved after.
    const bool msaa = msaaSamples > 1;
    sceneTarget.resize(width, height);

    if (msaa)
        msaaTarget.resize(width, height, msaaSamples);

    (msaa ? msaaTarget : sceneTarget).bind();
    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glEnable(GL_DEPTH_TEST);
    setCulling(faceCulling, GL_BACK);

    glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // A Vulkan backend would need a different projection here: depth range 0..1 instead of -1..1, and Y flipped.
    const glm::mat4 projection = glm::perspective(glm::radians(camera.fov), static_cast<float>(width) / height, camera.nearPlane, camera.farPlane);
    const glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + scene.forward(cameraEntity), kWorldUp);

    lit.use();
    lit.setMat4("projection", projection);
    lit.setMat4("view", view);
    lit.setVec3("viewPos", cameraPosition);
    lit.setBool("blinn", blinn);
    setSun(lit, sun);
    lit.setInt("lightCount", static_cast<int>(lightData.size()));

    lit.setMat4("lightSpace", lightSpace);
    lit.setBool("shadowsEnabled", castShadows);
    lit.setFloat("shadowBiasMin", shadowBiasMin);
    lit.setFloat("shadowBiasMax", shadowBiasMax);
    lit.setBool("pcf", pcf);
    shadowMap.bindDepth(2);

    // The environment that shiny materials reflect: the procedural sky, or the scene's sky texture.
    const bool hasSky = sky || scene.sky;
    lit.setBool("reflections", reflections && hasSky);
    lit.setBool("fresnel", fresnel);
    bindSky(scene, sky, 3);

    for (const Batch& batch : batches)
        drawBatch(batch);

    setCulling(faceCulling, GL_BACK);

    // A small sphere per enabled point light, in its color.
    lamp.use();
    lamp.setMat4("projection", projection);
    lamp.setMat4("view", view);

    for (size_t i = 0; i < points.size(); i++) {
        lamp.setVec3("lightColor", points[i].light.diffuse);
        lamp.setFloat("brightness", lampBrightness);
        sphere.drawInstanced(1, firstLamp + static_cast<GLuint>(i));
        drawCalls++;
        instances++;
    }

    // Sky last: its depth is 1.0, so the depth test skips every pixel an object already covered.
    // LEQUAL because it has to pass against the cleared depth, which is also 1.0.
    // Culling off because we're inside the cube, looking at its back faces.
    if (showSkybox && hasSky) {
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);

        skybox.use();
        skybox.setMat4("projection", projection);
        skybox.setMat4("view", view);
        skybox.setBool("sunDisk", sky != nullptr);

        if (sky) {
            skybox.setVec3("toSun", toSun);
            skybox.setVec3("sunColor", ProceduralSky::sunlight(*sky, toSun));
            skybox.setFloat("sunCosSize", std::cos(glm::radians(sky->sunSize * 0.5f)));
        }

        bindSky(scene, sky, 0);
        cube.draw();

        glDepthFunc(GL_LESS);
    }

    profiler.mark("Scene");

    if (msaa)
        msaaTarget.resolveTo(sceneTarget);

    profiler.mark("Resolve"); // also when MSAA is off, so the list of passes stays the same

    // The full-screen passes: one triangle each, no depth, no culling, filled even in wireframe mode.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBindVertexArray(emptyVao);

    // Bloom pass: the scene down and up the bloom chain.
    const GLuint bloomTexture = bloom ? bloomPass.apply(sceneTarget, width, height, bloomRadius) : 0;

    profiler.mark("Bloom"); // also when bloom is off, so the list of passes stays the same

    // Post pass: the HDR scene and the bloom through the post shader onto the window. It tone maps to 0..1 and
    // encodes to sRGB itself, so the result does not depend on the format of the window.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    post.use();
    post.setFloat("gamma", gamma);
    post.setBool("bloomEnabled", bloom);
    post.setFloat("bloomStrength", bloomStrength);
    post.setFloat("exposure", exposure);
    post.setInt("toneMap", toneMap);
    sceneTarget.bindColor(0);
    glBindTextureUnit(1, bloomTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    profiler.mark("Post");
    profiler.endFrame();
}

void Renderer::reloadChangedShaders() {
    for (Shader* shader : {&lit, &lamp, &post, &depth, &skybox, &bloomPass.down, &bloomPass.up, &proceduralSky.shader}) {
        if (!shader->reloadIfChanged() || !shader->error.empty())
            continue; // unchanged, or failed: a failure prints its own error

        std::printf("Reloaded %s\n", shader->name().c_str());

        if (shader == &proceduralSky.shader)
            proceduralSky.invalidate(); // the cubemap was drawn with the old shader
    }
}

void Renderer::bindSky(const Scene& scene, const Sky* sky, GLuint unit) const {
    if (sky)
        proceduralSky.bind(unit);
    else if (scene.sky)
        scene.sky->bind(unit);
}
