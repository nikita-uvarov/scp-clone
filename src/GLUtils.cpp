#include "GLUtils.h"

#include <set>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <sstream>

using namespace std;
using namespace sge;

bool GLSingleTextureMesh::checkIndices()
{
    if (vertices.size() != textureCoords.size())
        return false;
    
    for (int index: triangleIndices)
        if (index < 0 || index >= (int)vertices.size())
            return false;
        
    return true;
}

void GLSingleTextureMesh::render() const
{   
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBegin(GL_TRIANGLES);
    
    for (unsigned i = 0; i < triangleIndices.size(); i++)
    {
        const vec3& vertex = vertices[triangleIndices[i]];
        const vec2& texCoords = textureCoords[triangleIndices[i]];
        glTexCoord2d(texCoords.x, texCoords.y);
        glVertex3d(vertex.x, vertex.y, vertex.z);
    }
    
    glEnd();
}

// FIXME: remove this class & all mesh decomposition bicycles completely
struct UniqueVertex
{
    vec3 vertex;
    vec2 textureCoords;
    
    UniqueVertex(const GLSimpleFace& face, int index):
        vertex(face.vertices[index]), textureCoords(face.textureCoords[index])
    {}
    
    bool operator<(const UniqueVertex& v) const
    {
#define compareBy(field) if (!(abs(field - v.field) < 1e-3)) { return field < v.field; }
        compareBy(vertex.x);
        compareBy(vertex.y);
        compareBy(vertex.z);
        compareBy(textureCoords.x);
        compareBy(textureCoords.y);
#undef compareBy
        
        return false;
    }
    
    bool operator==(const UniqueVertex& v)
    {
#define compareBy(field) if (!(abs(field - v.field) < 1e-3)) { return false; }
        compareBy(vertex.x);
        compareBy(vertex.y);
        compareBy(vertex.z);
        compareBy(textureCoords.x);
        compareBy(textureCoords.y);
#undef compareBy
        
        return true;
    }
};

void GLSimpleMesh::render() const
{
    SDL_assert(!singleTextureMeshDecomposition.empty());
    
    for (const auto& subMesh: singleTextureMeshDecomposition)
        subMesh.render();
}

void GLSimpleMesh::decomposeIntoSingleTextureMeshes()
{
    SDL_assert(singleTextureMeshDecomposition.empty());
    
    sort(faces.begin(), faces.end(), [] (const GLSimpleFace& a, const GLSimpleFace& b) { return a.textureId < b.textureId; });
        
    set<UniqueVertex> currentVertexSet;
    unsigned from = 0;
    
    for (unsigned i = 0; i < faces.size() + 1; i++)
    {
        if (i == faces.size() || faces[i].textureId != faces[from].textureId)
        {
            // new texture starts
            GLSingleTextureMesh subMesh;
            subMesh.textureId = faces[from].textureId;
            
            for (auto it: currentVertexSet)
            {
                subMesh.vertices.push_back(it.vertex);
                subMesh.textureCoords.push_back(it.textureCoords);
            }
            
            vector<UniqueVertex> linear;
            copy(currentVertexSet.begin(), currentVertexSet.end(), back_inserter(linear));
     
#if 0
            printf("compressed to %d vertices\n", (int)linear.size());       
            for (auto key: linear)
                printf("%f %f %f  %f %f\n", key.vertex.x, key.vertex.y, key.vertex.z, key.textureCoords.x, key.textureCoords.y);
#endif
            
            for (unsigned j = from; j < i; j++)
            {
                vector<int> polygonIndices;
                
                for (unsigned vertexIndex = 0; vertexIndex < faces[j].vertices.size(); vertexIndex++)
                {
                    UniqueVertex key(faces[j], vertexIndex);
                    
                    // FIXME: again, remove this shit completely
                    auto vertexIt = lower_bound(linear.begin(), linear.end(), key);
                    
                    // Triggers if some points are close but not too close
                    SDL_assert(vertexIt != linear.end());
                    
                    int vertexId = (int)(vertexIt - linear.begin());
                    polygonIndices.push_back(vertexId);
                }
                
                SDL_assert(polygonIndices.size() >= 3);
                
                for (unsigned triangleVertex = 2; triangleVertex < polygonIndices.size(); triangleVertex++)
                {
                    subMesh.triangleIndices.push_back(polygonIndices[0]);
                    subMesh.triangleIndices.push_back(polygonIndices[triangleVertex - 1]);
                    subMesh.triangleIndices.push_back(polygonIndices[triangleVertex]);
                }
            }
            
            singleTextureMeshDecomposition.push_back(subMesh);
            
            currentVertexSet.clear();
            from = i;
        }
        
        if (i == faces.size()) break;
        
        for (unsigned vertexIndex = 0; vertexIndex < faces[i].vertices.size(); vertexIndex++)
        {
            UniqueVertex key(faces[i], vertexIndex);
            //printf("%f %f %f  %f %f\n", key.vertex.x, key.vertex.y, key.vertex.z, key.textureCoords.x, key.textureCoords.y);
            auto inserted = currentVertexSet.insert(key);
            if (!inserted.second)
            {
                key = *inserted.first;
                //printf("same as %f %f %f  %f %f\n", key.vertex.x, key.vertex.y, key.vertex.z, key.textureCoords.x, key.textureCoords.y);
            }
        }
    }
}

void colorf(vec3 vec)
{
    ftype color = cos(vec.x + vec.y + vec.z) * 0.25 + 0.6;
    glColor3d(color, color, color);
}

/*void SimplePhysicsEngine::dumpRenderNoModelview(bool overlapGeometry, bool renderCollisions)
{
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
 
    if (overlapGeometry)
        glPolygonOffset(-1, -1);
    else
        glPolygonOffset(1, 1);
    glEnable(GL_POLYGON_OFFSET_FILL);
    
    glBegin(GL_TRIANGLES);
    glColor3f(0.75, 0.75, 0.75);
    
    for (const auto& triangle: triangles)
    {
        if (triangle.collisionType == 0 || !renderCollisions)
        {
            colorf(triangle.a);
            glVertex3f(triangle.a.x, triangle.a.y, triangle.a.z);
            colorf(triangle.b);
            glVertex3f(triangle.b.x, triangle.b.y, triangle.b.z);
            colorf(triangle.c);
            glVertex3f(triangle.c.x, triangle.c.y, triangle.c.z);
        }
        else
        {   
            glColor3f(0.2f, 0.8f, 0);
            
            glVertex3f(triangle.a.x, triangle.a.y, triangle.a.z);
            glVertex3f(triangle.b.x, triangle.b.y, triangle.b.z);
            glVertex3f(triangle.c.x, triangle.c.y, triangle.c.z);
        }
    }
    
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    // otherwise it would blend, dunno why
    glColor3f(1, 1, 1);
    glDisable(GL_COLOR_MATERIAL);
}*/

void CharacterController::easyMove(MovementType type, ftype speed, ftype timeCoefficient)
{
    //if (abs(yAccel < 1e-3) return;
    speed = currentWalkSpeed;
    
    switch (type)
    {
        case MovementType::STRAFE_LEFT:
        case MovementType::STRAFE_RIGHT:
        {
            vec3 rightStrafe = glm::cross(direction, vec3(0, 1, 0));
            
            if (type == MovementType::STRAFE_RIGHT)
                position += rightStrafe * (speed * timeCoefficient);
            else
                position += -rightStrafe * (speed * timeCoefficient);
            
            break;
        }
        
        case MovementType::FORWARD:
        {
            position += direction * (speed * timeCoefficient);
            break;
        }
        
        case MovementType::BACKWARD:
        {
            position += -direction * (speed * timeCoefficient);
            break;
        }
        
        default: unreachable();
    }
}

/*void SimplePhysicsEngine::processPhysics()
{
    for (PhysicalBody* body: physicalBodies)
        processBody(*body);
}*/

void CharacterController::applySpeedCorrections(ftype timeCoefficient)
{
    if (newWalkSpeed < 1e3)
    {
        currentWalkSpeed = newWalkSpeed;
    }
    
    newWalkSpeed = 1e3;
    
    position += vec3(0, yAccel * timeCoefficient, 0);
    
    yAccel -= 0.003 * timeCoefficient;
    
    const ftype minYaccel = -0.07;
    if (yAccel < minYaccel) yAccel = minYaccel;
}

void CharacterController::applyYSmooth(ftype timeCoefficient)
{   
    ftype delta = abs(ySmooth - position.y);
    // delta > 2.0 -> low smoothing (coef = 0.25)
    // delta < 0.01 -> high smoothing (coef = 1)
    
    ftype highSmoothFloor = 0.01;
    ftype lowSmoothCeiling = 0.8;
    
    ftype smoothingCoefficient = 0;
    
    if (delta < highSmoothFloor)
        smoothingCoefficient = 1;
    else if (delta > lowSmoothCeiling)
        smoothingCoefficient = 0.25;
    else
    {
        ftype t = (delta - highSmoothFloor) / (lowSmoothCeiling - highSmoothFloor);
        smoothingCoefficient = 1 - 0.75 * t * t;
    }
    
    ftype dy = ySmooth * (smoothingCoefficient - 1) + position.y * (1 - smoothingCoefficient);
    ySmooth += dy * timeCoefficient;
}

vec4 extendPositionVector(vec3 pos)
{
    return vec4(pos.x, pos.y, pos.z, 1);
}

void dumpMatrix(mat4 m)
{
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 4; y++)
            printf("%.3lf ", m[x][y]);
        printf("\n");
    }
}

void CharacterController::processCollisionsAgainstMesh(GLPositionedMesh& mesh, CollisionPhase phase)
{
    //dumpMatrix(mesh.modelMatrix);
    // TODO: it is probably possible to use commented method of transformations
    // to achieve faster physics (no ftype matrix multiplications for each vertex)
    // though precision losses & jitter may follow
    // EDIT: now when doubles are used instead of floats,
    // no such problems should happen provided coordinates are not changed if there were no collision
    //position = vec3(mesh.inverseModelMatrix * extendPositionVector(position));
    //collisionOccured = false;
    
    //printf("matrix %lf %lf %lf\n", mesh.modelMatrix[0][3], mesh.modelMatrix[1][3], mesh.modelMatrix[2][3]);
    //printf("real pos %lf %lf %lf\n", position.x, position.y, position.z);
    //printf("eff pos %lf %lf %lf\n", position.x, position.y, position.z);
    
    for (GLSimpleFace& face: mesh.baseMesh->faces)
    {
        if (!face.isCollisionActive) continue;
        
        vector<vec3> vertices = face.vertices;
        for (auto& v: vertices)
            v = vec3(mesh.modelMatrix * extendPositionVector(v));
      
        for (int vertex = 2; vertex < (int)face.vertices.size(); vertex++)
        {
            if (phase == CollisionPhase::TRIANGLES)
                processCollisionsAgainstTriangle(face, vertices, 0, vertex - 1, vertex);
            else
            {
                processCollisionsAgainstSegment(vertices[0], vertices[vertex - 1]);
                processCollisionsAgainstSegment(vertices[vertex - 1], vertices[vertex]);
                processCollisionsAgainstSegment(vertices[0], vertices[vertex]);
            }
        }
    }
    
    //if (collisionOccured)
    //    position = vec3(mesh.modelMatrix * extendPositionVector(position));
}

void CharacterController::dumpRenderCollisionsAgainstMesh(GLPositionedMesh& mesh)
{
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_2D);
 
    if (false)
        glPolygonOffset(-1, -1);
    else
        glPolygonOffset(1, 1);
    glEnable(GL_POLYGON_OFFSET_FILL);
    
    glBegin(GL_TRIANGLES);
    glColor3f(0.75, 0.75, 0.75);
    
    for (GLSimpleFace& face: mesh.baseMesh->faces)
    {
        vector<vec3> vertices = face.vertices;
        for (auto& v: vertices)
            v = vec3(mesh.modelMatrix * extendPositionVector(v));
        
        glBegin(GL_POLYGON);
        for (auto it: vertices)
            glVertex3d(it.x, it.y, it.z);
        glEnd();
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    // otherwise it would blend, dunno why
    glColor3f(1, 1, 1);
    glDisable(GL_COLOR_MATERIAL);
}

ftype absTriangleSquare(const vec3& a, const vec3& b, const vec3& c)
{
    return glm::length(glm::cross(b - a, c - a)) * 0.5;
}

bool pointInTriangle(vec3 point, vec3 a, vec3 b, vec3 c)
{
    ftype square = absTriangleSquare(a, b, c);
    ftype withPoint =
        absTriangleSquare(a, b, point) +
        absTriangleSquare(b, c, point) +
        absTriangleSquare(c, a, point);
        
    if (abs(square - withPoint) < 1e-3)
        return true;
    
    return false;
}

void CharacterController::processCollisionsAgainstSegment(vec3 a, vec3 b)
{
    vec3& pos = position;
    ftype tProjected = glm::dot(b - a, pos - a);
    if (tProjected < 0 || tProjected > glm::length2(b - a))
        return;
    
    ftype distance = absTriangleSquare(a, b, pos) / glm::length(b - a) * 2;
    
    if (distance < radius)
    {
        vec3 projection = a + (tProjected / glm::length2(b - a)) * (b - a);
        //printf("got %lf %lf\n", glm::length(pos - projection), distance);
        //fflush(stdout);
        assert(abs(glm::length(pos - projection) - distance) < 1e-3);
        
        ftype inside = (radius - distance) / radius;
        
        vec3 fixVector = pos - projection;
        if (abs(inside) > 0.001)
        {
            pos = pos + fixVector * ((radius - distance) / radius);
            //collisionOccured = true;
        }
    }
}

void CharacterController::processCollisionsAgainstTriangle(GLSimpleFace& generatingFace, vector< vec3 >& transformedVertices, int i, int j, int k)
{   
    const vec3
        &triangleA = transformedVertices[i],
        &triangleB = transformedVertices[j],
        &triangleC = transformedVertices[k];
    
    // if projects on triangle
    //   calculate distance to plane
    //   if lower then radius
    //     apply normal correction
    
    vec3 normal = glm::cross(triangleB - triangleA, triangleC - triangleA);
    normal = glm::normalize(normal);
    
    // in Ax + By + Cz + D = 0 equation
    ftype D = -glm::dot(normal, triangleA);
    
    ftype distance = glm::dot(normal, position) + D;
    
    if (abs(distance) < radius)
    {
        // check that projects onto triangle
        // or one of the sides intersects sphere
        
        vec3 projection = position - normal * distance;
        
        bool intersection = pointInTriangle(projection, triangleA, triangleB, triangleC);
            
        if (intersection)
        {
            {
                ftype inside = 0;
                
                if (distance > 0)
                    inside = (radius - distance);
                else
                    inside = -radius - distance;
                
                if (generatingFace.isClimber && yAccel >= -0.004 && abs(inside) > 0.001)
                {
                    yAccel = 0.035;
                }
                
                if (abs(inside) > 0.001)
                {
                    position += normal * inside * 0.95;
                    //collisionOccured = true;
                }
                
                if (generatingFace.walkingSpeed > 1e-3)
                    newWalkSpeed = min(newWalkSpeed, generatingFace.walkingSpeed);
                
                if (abs(abs(normal.y) - 1) < 1e-3 && yAccel < 0)
                    yAccel = 0;
                
                //generatingFace.collisionType += 2;
            }
        }
    }
}

/*void GLSimpleMesh::extractPhysicalTriangles(vector<PhysicalTriangle>& physicalTriangles)
{
    for (GLSimpleFace& face: faces)
    {
        if (!face.isCollisionActive) continue;
        
        for (unsigned i = 2; i < face.vertices.size(); i++)
        {
            PhysicalTriangle triangle = { face.vertices[0], face.vertices[i - 1], face.vertices[i] };
            triangle.isClimber = face.isClimber;
            triangle.walkingSpeed = face.walkingSpeed;
            physicalTriangles.push_back(triangle);
        }
    }
}*/

void SimpleWorldContainer::addPositionedMesh(GLSimpleMesh& baseMesh, vec3 position)
{
    GLPositionedMesh mesh = GLPositionedMesh();
    mesh.baseMesh = &baseMesh;
    mesh.modelMatrix = glm::translate(mesh.modelMatrix, position);
    //mesh.inverseModelMatrix = glm::translate(mesh.inverseModelMatrix, -position);
    
    //dumpMatrix(mesh.modelMatrix);
    
    positionedMeshes.push_back(mesh);
}

void SimpleWorldContainer::renderWorld(mat4 projectionViewMatrix)
{
    for (GLPositionedMesh& positionedMesh: positionedMeshes)
    {
        mat4 meshMatrix = projectionViewMatrix * positionedMesh.modelMatrix;
        glLoadMatrixd(glm::value_ptr(meshMatrix));
        positionedMesh.baseMesh->render();
    }
}

void SimpleWorldContainer::processPhysics(CharacterController& controller, ftype dt)
{
    // physics engine is designed (read: constant tweaked) to use 1 / 60.0 step,
    // but theoretically it is possible to speed up and slow down
    
    ftype timeCoefficient = dt / (1 / 60.0);
    controller.applySpeedCorrections(timeCoefficient);
    
    for (int t = 0; t < 2; t++)
        for (GLPositionedMesh& mesh: positionedMeshes)
            controller.processCollisionsAgainstMesh(mesh, t == 0 ? CollisionPhase::TRIANGLES : CollisionPhase::SEGMENTS);
        
    if (controller.enableSmoothing)
        controller.applyYSmooth(timeCoefficient);
    else
        controller.ySmooth = controller.position.y;
}

void SimpleWorldContainer::dumpRenderPhysics(CharacterController& controller)
{
    for (GLPositionedMesh& mesh: positionedMeshes)
        controller.dumpRenderCollisionsAgainstMesh(mesh);
}

MeshMarker GLSimpleMesh::getObligatoryMarker(string name)
{
    MeshMarker* found = nullptr;
    
    for (MeshMarker& m: markers)
        if (m.name == name)
        {
            // multiple markers, 'obligatory' implies single
            SDL_assert(!found);
            found = &m;
        }
        
    SDL_assert(found);
    return *found;
}