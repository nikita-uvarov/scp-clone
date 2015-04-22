#include "Textures.h"
#include "SDL_image.h"

using namespace std;
using namespace sge;

int extendToPowerOfTwo(int value)
{
    int powerOfTwo = 1;
    
    while (powerOfTwo < value)
        powerOfTwo *= 2;
    
    return powerOfTwo;
}

unique_ptr<Texture> createTextureFromSurface(SDL_Surface* surface)
{
    unique_ptr<Texture> texturePtr(new Texture);
    Texture& texture = *texturePtr;
    
    texture.originalWidth = surface->w;
    texture.originalHeight = surface->h;
    
    texture.extendedWidth = extendToPowerOfTwo(texture.originalWidth);
    texture.extendedHeight = extendToPowerOfTwo(texture.originalHeight);

	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, texture.originalWidth, texture.originalHeight,
                                              32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (image == nullptr)
        return texturePtr;
    
	SDL_BlendMode saved_mode;
	SDL_GetSurfaceBlendMode(surface, &saved_mode);
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

	SDL_Rect all = { 0, 0, texture.originalWidth, texture.originalHeight };
	SDL_BlitSurface(surface, &all, image, &all);

	SDL_SetSurfaceBlendMode(surface, saved_mode);

	glGenTextures(1, &texture.openglId);
	glBindTexture(GL_TEXTURE_2D, texture.openglId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.originalWidth, texture.originalWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels);
	SDL_FreeSurface(image);

	return texturePtr;
}

unique_ptr<Texture> loadTexture(const char* fileName)
{
    SDL_Surface* surface = IMG_Load(fileName);
	verify(surface, "Unable to load texture '%s': '%s'\n", fileName, SDL_GetError());
    
	unique_ptr<Texture> texture(createTextureFromSurface(surface));
	SDL_FreeSurface(surface);
    
    return texture;
}

const Texture& TextureManager::retrieveTexture(string fileName)
{
    fileName = "resources/" + fileName;
    
    auto found = textureByName.find(fileName);
    if (found != textureByName.end())
        return *found->second;
    
    textureByName[fileName] = loadTexture(fileName.c_str());
    return *textureByName[fileName];
}

TextureManager& TextureManager::instance()
{
    static TextureManager instance;
    return instance;
}