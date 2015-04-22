#ifndef SCP_MESH_COLLECTION_H
#define SCP_MESH_COLLECTION_H

#include "Textures.h"

#include "GLUtils.h"
#include "glm/gtx/transform.hpp"

#include <map>
#include <memory>
#include <string>

namespace sge
{

GLSimpleMesh loadColladaMesh(const char* colladaFileName);

class StateMachineMeshEditor
{
protected :
    GLSimpleMesh* currentMesh;
    
    const Texture* currentTexture;
    GLSimpleFace currentFace;
    
    std::vector<mat4> transformStack;
    
    void loadIdentity()
    {
        transformStack.back() = mat4();
    }
    
    void translate(ftype x, ftype y, ftype z)
    {
        transformStack.back() = glm::translate(transformStack.back(), vec3(x, y, z));
    }
    
    void rotate(ftype radians, vec3 axis)
    {
        transformStack.back() = glm::rotate(transformStack.back(), radians, axis);
    }
    
    void scale(ftype x, ftype y, ftype z)
    {
        transformStack.back() = glm::scale(transformStack.back(), vec3(x, y, z));
    }
    
    void pushMatrix()
    {
        transformStack.push_back(transformStack.back());
    }
    
    void popMatrix()
    {
        assert(transformStack.size() > 1);
        transformStack.pop_back();
    }
    
    void initializeCurrentFace()
    {
        currentFace = GLSimpleFace();
        currentFace.walkingSpeed = 0.0;
        currentFace.isClimber = false;
        currentFace.isCollisionActive = true;
        
        currentFace.textureId = currentTexture ? currentTexture->openglId : 0;
    }
   
    void beginMesh(GLSimpleMesh* mesh)
    {
        currentMesh = mesh;
        initializeCurrentFace();
        transformStack.resize(1);
        loadIdentity();
    }
    
    void setTexture(std::string name)
    {
        currentTexture = &TextureManager::instance().retrieveTexture(name);
        currentFace.textureId = currentTexture->openglId;
    }
    
    void verts(const std::vector<vec3>& vertices)
    {
        for (auto v: vertices)
        {
            v = vec3(transformStack.back() * glm::vec4(v, 1.0));
            currentFace.vertices.push_back(v);
        }
    }
    
    void texCoords(const std::vector<vec2>& texCoords)
    {
        for (auto tc: texCoords)
        {
            tc.x = tc.x * currentTexture->getMaxU();
            tc.y = tc.y * currentTexture->getMaxV();
            currentFace.textureCoords.push_back(tc);
        }
    }
    
    void makeClimber()
    {
        currentFace.isClimber = true;
    }
    
    void makeCollisionInactive()
    {
        currentFace.isCollisionActive = false;
    }
    
    void setWalkingSpeed(ftype walkingSpeed)
    {
        currentFace.walkingSpeed = walkingSpeed;
    }
    
    void finishFace()
    {
        SDL_assert(currentFace.vertices.size() > 2);
        SDL_assert(currentFace.textureCoords.size() == currentFace.vertices.size());
        currentMesh->faces.push_back(currentFace);
        initializeCurrentFace();
    }
    
    void markCurrentPosition(std::string name)
    {
        const mat4& currentMatrix = transformStack.back();
        
        MeshMarker marker;
        marker.name = name;
        marker.direction = vec3(currentMatrix * vec4(0, 0, -1, 0));
        marker.position = vec3(currentMatrix * vec4(0, 0, 0, 1));
        
        currentMesh->markers.push_back(marker);
    }
    
    void finishMesh()
    {
        currentMesh->decomposeIntoSingleTextureMeshes();
    }
    
    // pushMatrix / popMatrix
    // translate
    // setTexture: current all-face texture
    // verts( { }, { }, { }
    // texCoords ( { }, { }, { } )
    // makeClimber, makeCollisionInactive, setWalkingSpeed
    // finishFace
    
    // handy quad adding functions for quads parallel to XY, YZ or ZX
    
    void addQuad(vec3 a, vec3 b, ftype textureScale = 0.0);
    
public :
    StateMachineMeshEditor(): currentTexture(nullptr) {}
};

class ScpMeshCollection : public StateMachineMeshEditor
{
public :
    GLSimpleMesh cubeMesh;
    GLSimpleMesh staircase;
    GLSimpleMesh plane;
    
    void createStaircaseMesh();
    void createPlaneMesh();
    
    void loadMeshes();
    
    GLSimpleMesh createStaircase(ftype horizOne, ftype vertOne, ftype horizTwo, ftype vertTwo, int nSteps = 15);
};

}

#endif // SCP_MESH_COLLECTION_H