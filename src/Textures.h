#ifndef SGE_TEXTURES_H
#define SGE_TEXTURES_H

#include "Common.h"

#include <map>
#include <string>
#include <memory>

namespace sge
{
 
class Texture
{
public :
    GLuint openglId;
    int originalWidth, originalHeight;
    int extendedWidth, extendedHeight;
    
    Texture(): openglId(0) {}
    Texture(const Texture& t) = delete;
    
    ~Texture()
    {
        glDeleteTextures(1, &openglId);
    }
    
    bool isValid() const { return openglId != 0; }
    
    ftype getMaxU() const { return originalWidth / (ftype)extendedWidth; }
    ftype getMaxV() const { return originalHeight / (ftype)extendedHeight; }
};

class TextureManager
{
    std::map< std::string, std::unique_ptr<Texture> > textureByName;
    
    TextureManager() {}
public :
    
    static TextureManager& instance();
    
    const Texture& retrieveTexture(std::string fileName);
};

}

#endif // SGE_TEXTURES_H