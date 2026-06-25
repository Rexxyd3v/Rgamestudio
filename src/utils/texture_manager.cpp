#include "texture_manager.h"

std::map<std::string, Texture2D> TextureManager::textures;

Texture2D TextureManager::GetTexture(const std::string& path) {
    if (textures.find(path) == textures.end()) {
        textures[path] = LoadTexture(path.c_str());
    }
    return textures[path];
}

void TextureManager::UnloadAll() {
    for (auto& pair : textures) {
        UnloadTexture(pair.second);
    }
    textures.clear();
}
