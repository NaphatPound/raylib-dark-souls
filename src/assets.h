#pragma once
#include "raylib.h"
#include <string>

#ifndef ASSET_DIR
#define ASSET_DIR "assets"
#endif

namespace assets {
// Absolute path to an asset relative to the baked-in asset root.
std::string path(const std::string& rel);
// Cached texture load (returns a 1x1 white fallback if the file is missing).
Texture2D texture(const std::string& rel);
void unload_all();
}
