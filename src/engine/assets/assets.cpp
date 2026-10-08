#include "assets/assets.h"

#include "core/paths.h"

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
    return textures.get(assetRoot() + "textures/" + file, srgb);
}

const Model& Assets::model(const std::string& file) {
    std::unique_ptr<Model>& slot = models[file];

    if (!slot)
        slot = std::make_unique<Model>(assetRoot() + "models/" + file, textures);

    return *slot;
}

const Cubemap& Assets::cubemap(const std::string& directory) {
    std::unique_ptr<Cubemap>& slot = cubemaps[directory];

    if (!slot)
        slot = std::make_unique<Cubemap>((assetRoot() + "textures/" + directory).c_str());

    return *slot;
}

template <typename T>
static std::string nameIn(const std::unordered_map<std::string, std::unique_ptr<T>>& owned, const T& asset) {
    for (const auto& [name, pointer] : owned) {
        if (pointer.get() == &asset)
            return name;
    }

    throw std::runtime_error("Asset isn't owned by Assets");
}

std::string Assets::name(const Mesh& mesh) const {
    return nameIn(meshes, mesh);
}

std::string Assets::name(const Texture& texture) const {
    const std::string path = textures.pathOf(texture);
    const std::string root = assetRoot() + "textures/";

    if (path.compare(0, root.size(), root) != 0)
        throw std::runtime_error("Texture isn't under the textures folder: " + path);

    return path.substr(root.size());
}

std::string Assets::name(const Model& model) const {
    return nameIn(models, model);
}

std::string Assets::name(const Cubemap& cubemap) const {
    return nameIn(cubemaps, cubemap);
}
