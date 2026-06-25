#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <raylib.h>
#include <string>
#include <map>

class TextureManager {
public:
    static Texture2D GetTexture(const std::string& path);
    static void UnloadAll();

private:
    static std::map<std::string, Texture2D> textures;
};

#endif // TEXTURE_MANAGER_H
