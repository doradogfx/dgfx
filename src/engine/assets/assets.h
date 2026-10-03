#pragma once

#include "assets/model.h"
#include "renderer/cubemap.h"
#include "renderer/mesh.h"
#include "renderer/texture.h"

#include <memory>
#include <string>
#include <unordered_map>

// Owns every mesh, texture, model and cubemap, loading each one the first time it's asked for and handing
// out the same object after that. Components keep plain pointers into it: nothing is ever unloaded, and
// unique_ptr keeps each object at a fixed address as the maps grow.
class Assets {
public:
    const Mesh& mesh(const std::string& name); // built in: "cube", "sphere"
    // ponytail: the cache keys by path only, so one file asked for as both sRGB and linear gives whichever came first.
    const Texture& texture(const std::string& file, bool srgb); // relative to textures/
    const Model& model(const std::string& file);                // relative to models/
    const Cubemap& cubemap(const std::string& directory);       // relative to textures/

    // The reverse: the name an asset was loaded under, for saving scenes. Throws if Assets doesn't own it.
    // ponytail: scans the maps, fine for an occasional save.
    std::string name(const Mesh& mesh) const;
    std::string name(const Texture& texture) const; // file relative to textures/
    std::string name(const Model& model) const;
    std::string name(const Cubemap& cubemap) const;

private:
    TextureCache textures; // declared before the models, so it outlives them
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;
    std::unordered_map<std::string, std::unique_ptr<Model>> models;
    std::unordered_map<std::string, std::unique_ptr<Cubemap>> cubemaps;
};
