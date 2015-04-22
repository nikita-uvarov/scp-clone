#ifndef COLLADA_MESH_LOADER
#define COLLADA_MESH_LOADER

#include <string>

// normal mesh architecture:
// vertices array
// several indices arrays for multiple materials
// texture are stored by texture manager

class TextureReference
{
public :
    string fileName;
    GLuint loadedId;
};

class MeshMaterial
{
public :
    TextureReference diffuseTexture;
    TextureReference lightingTexture;
};

class ColladaElement
{
    vector<ColladaElement*> children;
    string id, sid;
};

class ColladaMeshLoader
{
};

#endif // COLLADA_MESH_LOADER