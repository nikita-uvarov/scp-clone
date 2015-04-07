#include "GLUtils.h"
#include "SDLUtils.h"
#include "SDL_image.h"

#include <glm/gtx/norm.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <set>
#include <cmath>
#include <vector>
#include <algorithm>

using namespace std;

int extendToPowerOfTwo(int number)
{
    int twoPower = 1;
    
    while (twoPower < number)
        twoPower *= 2;
    
    return twoPower;
}

GLTexture createTextureFromSurface(SDL_Surface* surface)
{
    // is invalid until gl id is assigned
    GLTexture texture = {};
    texture.originalWidth = surface->w;
    texture.originalHeight = surface->h;
    
    texture.extendedWidth = extendToPowerOfTwo(texture.originalWidth);
    texture.extendedHeight = extendToPowerOfTwo(texture.originalHeight);

	SDL_Surface* image = SDL_CreateRGBSurface(SDL_SWSURFACE, texture.originalWidth, texture.originalHeight,
                                              32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (image == nullptr)
        return texture;
    
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

	return texture;
}

GLTexture loadTexture(const char* fileName)
{
    SDL_Surface* surface = IMG_Load(fileName);
	SDL_VerifyResult(surface, "Unable to load texture '%s': '%s'\n", fileName, SDL_GetError());
    
	GLTexture texture = createTextureFromSurface(surface);
	SDL_FreeSurface(surface);
    
    return texture;
}

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
        const glm::vec3& vertex = vertices[triangleIndices[i]];
        const glm::vec2& texCoords = textureCoords[triangleIndices[i]];
        glTexCoord2f(texCoords.x, texCoords.y);
        glVertex3f(vertex.x, vertex.y, vertex.z);
    }
    
    glEnd();
}

struct UniqueVertex
{
    glm::vec3 vertex;
    glm::vec2 textureCoords;
    
    UniqueVertex(const GLSimpleFace& face, int index):
        vertex(face.vertices[index]), textureCoords(face.textureCoords[index])
    {}
    
    bool operator<(const UniqueVertex& v) const
    {
        // FIXME: 1e-3
#define compareBy(field) if (!(abs(field - v.field) < 1e-3)) { return field < v.field; }
        compareBy(vertex.x);
        compareBy(vertex.y);
        compareBy(vertex.z);
        compareBy(textureCoords.x);
        compareBy(textureCoords.y);
#undef compareBy
        
        return false;
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

void colorf(glm::vec3 vec)
{
    GLfloat color = cos(vec.x + vec.y + vec.z) * 0.25f + 0.6f;
    glColor3f(color, color, color);
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

void CharacterController::easyMove(MovementType type, double speed, double dt)
{
    //if (abs(yAccel < 1e-3) return;
    speed = currentWalkSpeed;
    
    switch (type)
    {
        case MovementType::STRAFE_LEFT:
        case MovementType::STRAFE_RIGHT:
        {
            glm::vec3 rightStrafe = glm::cross(direction, glm::vec3(0, 1, 0));
            
            if (type == MovementType::STRAFE_RIGHT)
                position += rightStrafe * (GLfloat)(speed * dt);
            else
                position += -rightStrafe * (GLfloat)(speed * dt);
            
            break;
        }
        
        case MovementType::FORWARD:
        {
            position += direction * (GLfloat)(speed * dt);
            break;
        }
        
        case MovementType::BACKWARD:
        {
            position += -direction * (GLfloat)(speed * dt);
            break;
        }
        
        default: SDL_assert(!"impossible");
    }
}

/*void SimplePhysicsEngine::processPhysics()
{
    for (PhysicalBody* body: physicalBodies)
        processBody(*body);
}*/

void CharacterController::applySpeedCorrections()
{
    if (newWalkSpeed < 1e3)
    {
        currentWalkSpeed = newWalkSpeed;
    }
    
    newWalkSpeed = 1e3;
    
    position += glm::vec3(0, yAccel, 0);
    
    yAccel -= 0.003;
    
    const double minYaccel = -0.07;
    if (yAccel < minYaccel) yAccel = minYaccel;
}

glm::vec4 extendPositionVector(glm::vec3 pos)
{
    return glm::vec4(pos.x, pos.y, pos.z, 1);
}

void dumpMatrix(glm::mat4 m)
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
    // to achieve faster physics (no double matrix multiplications for each vertex)
    // though precision losses & jitter may follow
    //position = glm::vec3(mesh.inverseModelMatrix * extendPositionVector(position));
    //collisionOccured = false;
    
    //printf("matrix %lf %lf %lf\n", mesh.modelMatrix[0][3], mesh.modelMatrix[1][3], mesh.modelMatrix[2][3]);
    //printf("real pos %lf %lf %lf\n", position.x, position.y, position.z);
    //printf("eff pos %lf %lf %lf\n", position.x, position.y, position.z);
    
    for (GLSimpleFace& face: mesh.baseMesh->faces)
    {
        vector<glm::vec3> vertices = face.vertices;
        for (auto& v: vertices)
            v = glm::vec3(mesh.modelMatrix * extendPositionVector(v));
      
        for (int vertex = 2; vertex < (int)face.vertices.size(); vertex++)
        {
            if (phase == CollisionPhase::TRIANGLES)
                processCollisionsAgainstTriangle(face, vertices, 0, vertex - 1, vertex);
            else
            {
                // FIXME:
                processCollisionsAgainstSegment(vertices[0], vertices[vertex - 1]);
                processCollisionsAgainstSegment(vertices[vertex - 1], vertices[vertex]);
                processCollisionsAgainstSegment(vertices[0], vertices[vertex]);
            }
        }
    }
    
    //if (collisionOccured)
    //    position = glm::vec3(mesh.modelMatrix * extendPositionVector(position));
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
        vector<glm::vec3> vertices = face.vertices;
        for (auto& v: vertices)
            v = glm::vec3(mesh.modelMatrix * extendPositionVector(v));
        
        glBegin(GL_POLYGON);
        for (auto it: vertices)
            glVertex3f(it.x, it.y, it.z);
        glEnd();
    }
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    
    // otherwise it would blend, dunno why
    glColor3f(1, 1, 1);
    glDisable(GL_COLOR_MATERIAL);
}

/*void SimplePhysicsEngine::processBody(PhysicalBody& body)
{
    
    for (PhysicalTriangle& triangle: triangles)
    {
        triangle.collisionType = 0;
        processBodyTriangle(body, triangle);
    }
    
    for (const PhysicalTriangle& triangle: triangles)
    {
        processBodySegment(body, triangle.a, triangle.b);
        processBodySegment(body, triangle.b, triangle.c);
        processBodySegment(body, triangle.c, triangle.a);
    }
    
    if (body.newWalkSpeed < 1e3)
    {
        body.currentWalkSpeed = body.newWalkSpeed;
    }
}*/

double absTriangleSquare(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    return glm::length(glm::cross(b - a, c - a)) * 0.5;
}

bool pointInTriangle(glm::vec3 point, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
    double square = absTriangleSquare(a, b, c);
    double withPoint =
        absTriangleSquare(a, b, point) +
        absTriangleSquare(b, c, point) +
        absTriangleSquare(c, a, point);
        
    if (abs(square - withPoint) < 1e-3)
        return true;
    
    return false;
}

void CharacterController::processCollisionsAgainstSegment(glm::vec3 a, glm::vec3 b)
{
    glm::vec3& pos = position;
    double tProjected = glm::dot(b - a, pos - a);
    if (tProjected < 0 || tProjected > glm::length2(b - a))
        return;
    
    double distance = absTriangleSquare(a, b, pos) / glm::length(b - a) * 2;
    
    if (distance < radius)
    {
        glm::vec3 projection = a + (GLfloat)(tProjected / glm::length2(b - a)) * (b - a);
        //printf("got %lf %lf\n", glm::length(pos - projection), distance);
        //fflush(stdout);
        assert(abs(glm::length(pos - projection) - distance) < 1e-3);
        
        double inside = (radius - distance) / radius;
        
        glm::vec3 fixVector = pos - projection;
        if (abs(inside) > 0.001)
        {
            pos = pos + fixVector * (GLfloat)((radius - distance) / radius);
            collisionOccured = true;
        }
    }
}

void CharacterController::processCollisionsAgainstTriangle(GLSimpleFace& generatingFace, vector< glm::vec3 >& transformedVertices, int i, int j, int k)
{   
    const glm::vec3
        &triangleA = transformedVertices[i],
        &triangleB = transformedVertices[j],
        &triangleC = transformedVertices[k];
    
    // if projects on triangle
    //   calculate distance to plane
    //   if lower then radius
    //     apply normal correction
    
    glm::vec3 normal = glm::cross(triangleB - triangleA, triangleC - triangleA);
    normal = glm::normalize(normal);
    
    // in Ax + By + Cz + D = 0 equation
    double D = -glm::dot(normal, triangleA);
    
    double distance = glm::dot(normal, position) + D;
    
    if (abs(distance) < radius)
    {
        // check that projects onto triangle
        // or one of the sides intersects sphere
        
        glm::vec3 projection = position - normal * (GLfloat)distance;
        
        bool intersection = pointInTriangle(projection, triangleA, triangleB, triangleC);
            
        if (intersection)
        {
            {
                double inside = 0;
                
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
                    position += normal * (GLfloat)inside * 0.95f;
                    collisionOccured = true;
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

void SimpleWorldContainer::addPositionedMesh(GLSimpleMesh& baseMesh, glm::vec3 position)
{
    GLPositionedMesh mesh = GLPositionedMesh();
    mesh.baseMesh = &baseMesh;
    mesh.modelMatrix = glm::translate(mesh.modelMatrix, position);
    //mesh.inverseModelMatrix = glm::translate(mesh.inverseModelMatrix, -position);
    
    //dumpMatrix(mesh.modelMatrix);
    
    positionedMeshes.push_back(mesh);
}

void SimpleWorldContainer::renderWorld(glm::mat4 projectionViewMatrix)
{
    for (GLPositionedMesh& positionedMesh: positionedMeshes)
    {
        glLoadMatrixf(glm::value_ptr(projectionViewMatrix * positionedMesh.modelMatrix));
        positionedMesh.baseMesh->render();
    }
}

void SimpleWorldContainer::processPhysics(CharacterController& controller)
{
    controller.applySpeedCorrections();
    
    for (int t = 0; t < 2; t++)
        for (GLPositionedMesh& mesh: positionedMeshes)
            controller.processCollisionsAgainstMesh(mesh, t == 0 ? CollisionPhase::TRIANGLES : CollisionPhase::SEGMENTS);
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
