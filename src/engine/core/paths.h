#pragma once

#include <string>

// The folder that holds shaders/, textures/, models/, scenes/ and sounds/, with a '/' at the end.
// Editor build: the source tree, so a saved scene goes into the repository.
// Build without the editor: the folder of the exe. The build copies the assets there.
const std::string& assetRoot();
