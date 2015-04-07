#ifndef SCP_MESH_COLLECTION_H
#define SCP_MESH_COLLECTION_H

#include "GLUtils.h"
#include "glm/gtx/transform.hpp"

#include <map>
#include <memory>
#include <string>

class SimpleTextureManager
{
    std::map< std::string, std::unique_ptr<GLTexture> > textureByName;
public :
    
    GLTexture& get(std::string name);
};

class StateMachineMeshEditor
{
protected :
    SimpleTextureManager& textureManager;
    GLSimpleMesh* currentMesh;
    
    GLTexture currentTexture;
    GLSimpleFace currentFace;
    
    std::vector<glm::mat4> transformStack;
    
    void loadIdentity()
    {
        transformStack.back() = glm::mat4();
    }
    
    void translate(double x, double y, double z)
    {
        transformStack.back() = glm::translate(transformStack.back(), glm::vec3((GLfloat)x, (GLfloat)y, (GLfloat)z));
    }
    
    void rotate(double radians, glm::vec3 axis)
    {
        transformStack.back() = glm::rotate(transformStack.back(), (GLfloat)radians, axis);
    }
    
    void scale(double x, double y, double z)
    {
        transformStack.back() = glm::scale(transformStack.back(), glm::vec3((GLfloat)x, (GLfloat)y, (GLfloat)z));
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
        currentFace.textureId = currentTexture.openglId;
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
        currentTexture = textureManager.get(name);
        currentFace.textureId = currentTexture.openglId;
    }
    
    void verts(const std::vector<glm::vec3>& vertices)
    {
        for (auto v: vertices)
        {
            v = glm::vec3(transformStack.back() * glm::vec4(v, 1.0));
            currentFace.vertices.push_back(v);
        }
    }
    
    void texCoords(const std::vector<glm::vec2>& texCoords)
    {
        for (auto tc: texCoords)
        {
            tc.x = tc.x * currentTexture.getMaxU();
            tc.y = tc.y * currentTexture.getMaxV();
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
    
    void setWalkingSpeed(double walkingSpeed)
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
        const glm::mat4& currentMatrix = transformStack.back();
        
        MeshMarker marker;
        marker.name = name;
        marker.direction = glm::vec3(currentMatrix * glm::vec4(0, 0, -1, 0));
        marker.position = glm::vec3(currentMatrix * glm::vec4(0, 0, 0, 1));
        
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
    
    void addQuad(glm::vec3 a, glm::vec3 b, double textureScale = 0.0);
    
public :
    
    StateMachineMeshEditor(SimpleTextureManager& textureManager): textureManager(textureManager) {}
};

class ScpMeshCollection : public StateMachineMeshEditor
{
public :
    GLSimpleMesh cubeMesh;
    GLSimpleMesh staircase;
    
    ScpMeshCollection(SimpleTextureManager& textureManager): StateMachineMeshEditor(textureManager) {}
    
    void createStaircaseMesh();
    
    void loadMeshes();
};

//GLSimpleMesh createCubeMesh(GLuint textureId, GLfloat textureMaxX = 1.0f, GLfloat textureMaxY = 1.0f);

#endif // SCP_MESH_COLLECTION_H