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
    
    bool isCollisionActive;
    double walkingSpeed;
    bool isClimber;
};

class PhysicalTriangle
{
public :
    glm::vec3 a, b, c;
    double walkingSpeed;
    int collisionType;
    bool isClimber;
};

class GLSimpleMesh
{
public :
    std::vector<GLSimpleFace> faces;
    std::vector<GLSingleTextureMesh> singleTextureMeshDecomposition;
    
    // may sort faces
    void decomposeIntoSingleTextureMeshes();
    
    void extractPhysicalTriangles(std::vector<PhysicalTriangle>& physicalTriangles);
    
    void render() const;
};

class GLPositionedMesh
{
public :
    GLSimpleMesh* baseMesh;
    
    // inverse matrix is used in physics calculations
    glm::mat4 modelMatrix, inverseModelMatrix;
};

enum class MovementType
{
    FORWARD,
    BACKWARD,
    STRAFE_LEFT,
    STRAFE_RIGHT
};

enum class CollisionPhase
{
    TRIANGLES,
    SEGMENTS
};

class CharacterController
{
    // might not be a good idea...
    glm::vec3 effectivePosition;
    bool collisionOccured;
    
    void processCollisionsAgainstTriangle(GLSimpleFace& generatingFace, std::vector<glm::vec3>& transformedVertices, int i, int j, int k);
    void processCollisionsAgainstSegment(glm::vec3 a, glm::vec3 b);
public :
    glm::vec3 position;
    glm::vec3 direction;
    double radius;
    
    double yAccel;
    double currentWalkSpeed;
    double newWalkSpeed;
    
    void easyMove(MovementType type, double speed, double dt);
    
    void applySpeedCorrections();
    
    // two passes are required
    void processCollisionsAgainstMesh(GLPositionedMesh& mesh, CollisionPhase phase);
    void dumpRenderCollisionsAgainstMesh(GLPositionedMesh& mesh);
};

// a no-acceleration physics engine
/*class SimplePhysicsEngine
{
    bool enableGravity;
    
    //void processBody(PhysicalBody& body);
    //void processBodyTriangle(PhysicalBody& body, PhysicalTriangle& triangle);
    //void processBodySegment(PhysicalBody& body, glm::vec3 a, glm::vec3 b);
    
public :
    std::vector<PhysicalTriangle> triangles;
    //std::vector<PhysicalBody*> physicalBodies;
    
    //void processPhysics();
    
    void dumpRenderNoModelview(bool overlapGeometry, bool renderCollisions);
};*/

/*class SimpleResourceManager
{
public :
    vector<GLSimpleMesh> meshesLoaded;
};*/

// simply provides group interface
class SimpleWorldContainer
{
public :
    std::vector<GLPositionedMesh> positionedMeshes;
    
    void addPositionedMesh(GLSimpleMesh& baseMesh, glm::vec3 position);
    
    void renderWorld(glm::mat4 projectionViewMatrix);
    void processPhysics(CharacterController& controller);
    void dumpRenderPhysics(CharacterController& controller);
};

#endif // GL_UTILS_H
