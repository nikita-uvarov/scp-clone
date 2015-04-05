#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "SDL.h"
#include "SDL_opengl.h"

#include "glm/glm.hpp"

#include <vector>

class GLTexture
{
public :
    GLuint openglId;
    int originalWidth, originalHeight;
    int extendedWidth, extendedHeight;
    
    bool isValid() { return openglId != 0; }
    
    GLfloat getMaxU() { return (GLfloat)originalWidth / (GLfloat)extendedWidth; }
    GLfloat getMaxV() { return (GLfloat)originalHeight / (GLfloat)extendedHeight; }
};

GLTexture loadTexture(const char* fileName);

// It is possible to render multi-texture meshes without context switches
// By supplying the vertex shader texture ids (~kinda)
// We split into similar texture parts instead
class GLSingleTextureMesh
{
public :
    GLuint textureId;
    
    std::vector<glm::vec3> vertices;
    
    // attached to vertices
    std::vector<glm::vec2> textureCoords;
    
    std::vector<int> triangleIndices;
    
    void render() const;
    
    bool checkIndices();
};

class GLSimpleFace
{
public :
    GLuint textureId;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> textureCoords;
};

class GLSimpleMesh
{
public :
    std::vector<GLSimpleFace> faces;
    std::vector<GLSingleTextureMesh> singleTextureMeshDecomposition;
    
    // may sort faces
    void decomposeIntoSingleTextureMeshes();
    
    void render() const;
};

GLSimpleMesh createCubeMesh(GLuint textureId, GLfloat textureMaxX = 1.0f, GLfloat textureMaxY = 1.0f);

class PhysicalTriangle
{
public :
    glm::vec3 a, b, c;
};

enum class MovementType
{
    FORWARD,
    BACKWARD,
    STRAFE_LEFT,
    STRAFE_RIGHT
};

class PhysicalBody
{
public :
    glm::vec3 position;
    glm::vec3 direction;
    double radius;
    
    void easyMove(MovementType type, double speed, double dt);
};

// a no-acceleration physics engine
class SimplePhysicsEngine
{
    void processBody(PhysicalBody& body);
    void processBodyTriangle(PhysicalBody& body, const PhysicalTriangle& triangle);
    void processBodySegment(PhysicalBody& body, glm::vec3 a, glm::vec3 b);
    
public :
    std::vector<PhysicalTriangle> triangles;
    std::vector<PhysicalBody*> physicalBodies;
    
    void processPhysics();
    
    void dumpRenderNoModelview(bool overlapGeometry);
};

#endif // GL_UTILS_H
