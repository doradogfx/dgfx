#include "assets/assets.h"

#include <stdexcept>

const Mesh& Assets::mesh(const std::string& name) {
    std::unique_ptr<Mesh>& slot = meshes[name];

    if (!slot) {
        if (name == "cube")
            slot = std::make_unique<Mesh>(makeCube());
        else if (name == "sphere")
            slot = std::make_unique<Mesh>(makeSphere());
        else
            throw std::runtime_error("Unknown mesh " + name);
    }

    return *slot;
}

const Texture& Assets::texture(const std::string& file, bool srgb) {
    return textures.get(TEXTURE_DIR + file, srgb);
}

const Model& Assets::model(const std::string& file) {
    std::unique_ptr<Model>& slot = models[file];

    if (!slot)
        slot = std::make_unique<Model>(MODEL_DIR + file, textures);

    return *slot;
}

const Cubemap& Assets::cubemap(const std::string& directory) {
    std::unique_ptr<Cubemap>& slot = cubemaps[directory];

    if (!slot)
        slot = std::make_unique<Cubemap>((TEXTURE_DIR + directory).c_str());

    return *slot;
}
