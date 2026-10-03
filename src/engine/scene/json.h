#pragma once

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

using Json = nlohmann::json;

// glm vectors are written as [x, y, z].
namespace glm {

inline void to_json(Json& j, const vec3& v) {
    j = Json::array({v.x, v.y, v.z});
}

inline void from_json(const Json& j, vec3& v) {
    v = vec3(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
}

} // namespace glm
