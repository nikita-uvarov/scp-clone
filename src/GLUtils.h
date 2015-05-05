#ifndef GL_UTILS_H
#define GL_UTILS_H

#include "Common.h"

#include <vector>
#include <string>

namespace sge
{

// It is possible to render multi-texture meshes without context switches
// By supplying the vertex shader texture ids (~kinda)
// We split into similar texture parts instead
class GLSingleTextureMesh
{
public :
    GLuint textureId = 0;
    
    std::vector<vec3> vertices;
    
    // attached to vertices
    std::vector<vec2> textureCoords;
    
    std::vector<int> triangleIndices;
    
    void render() const;
    
    bool checkIndices();
};

class GLSimpleFace
{
public :
    GLuint textureId = 0;
    std::vector<vec3> vertices;
    std::vector<vec2> textureCoords;
    
    bool isCollisionActive = false;
    ftype walkingSpeed = 0;
    bool isClimber = false;
};

class MeshMarker
{
public :
    std::string name;
    vec3 position, direction;
};

class GLSimpleMesh
{
public :
    std::vector<GLSimpleFace> faces;
    std::vector<GLSingleTextureMesh> singleTextureMeshDecomposition;
    std::vector<MeshMarker> markers;
    
    // may sort faces
    void decomposeIntoSingleTextureMeshes();
    
    //void extractPhysicalTriangles(std::vector<PhysicalTriangle>& physicalTriangles);
    
    void render() const;
    
    MeshMarker getObligatoryMarker(std::string name);
};

class GLPositionedMesh
{
public :
    GLSimpleMesh* baseMesh = nullptr;
    
    mat4 modelMatrix;
    
    // inverse matrix is used in physics calculations
    //mat4 inverseModelMatrix;
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
    //vec3 effectivePosition;
    //bool collisionOccured;
    
    void processCollisionsAgainstTriangle(GLSimpleFace& generatingFace, std::vector<vec3>& transformedVertices, int i, int j, int k);
    void processCollisionsAgainstSegment(vec3 a, vec3 b);
public :
    vec3 position;
    
    bool enableSmoothing = false;
    ftype ySmooth = 0;
    
    vec3 direction;
    ftype radius = 0;
    
    ftype yAccel = 0;
    ftype currentWalkSpeed = 0;
    ftype newWalkSpeed = 0;
    
    CharacterController():
        position(), enableSmoothing(false), ySmooth(position.y), direction(0, 0, -1), radius(1),
        yAccel(0), currentWalkSpeed(0), newWalkSpeed(0) {}
    
    void easyMove(MovementType type, ftype speed, ftype timeCoefficient);
    
    void applySpeedCorrections(ftype timeCoefficient);
    
    // two passes are required
    void processCollisionsAgainstMesh(GLPositionedMesh& mesh, CollisionPhase phase);
    void dumpRenderCollisionsAgainstMesh(GLPositionedMesh& mesh);
    
    void applyYSmooth(ftype timeCoefficient);
};

// simply provides group interface
class SimpleWorldContainer
{
public :
    std::vector<GLPositionedMesh> positionedMeshes;
    
    void addPositionedMesh(GLSimpleMesh& baseMesh, vec3 position);
    
    void renderWorld(mat4 projectionViewMatrix);
    void processPhysics(CharacterController& controller, ftype dt);
    void dumpRenderPhysics(CharacterController& controller);
};

}

#endif // GL_UTILS_H
