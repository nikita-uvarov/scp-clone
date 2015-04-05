#include "GLUtils.h"
#include "SDLUtils.h"
#include "SDL_image.h"

#include <glm/gtx/norm.hpp>

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

GLSimpleMesh createCubeMesh(GLuint textureId, GLfloat textureMaxX, GLfloat textureMaxY)
{
    GLSimpleMesh cube;
    
    vector<glm::vec3> cubeVerts;
    
    for (int x = -1; x <= 1; x += 2)
        for (int y = -1; y <= 1; y += 2)
            for (int z = -1; z <= 1; z += 2)
                cubeVerts.push_back(glm::vec3(x, y, z));
            
    assert(cubeVerts.size() == 8);
    
    for (int i = 0; i < 8; i++)
        for (int j = i + 1; j < 8; j++)
            for (int k = j + 1; k < 8; k++)
                for (int l = k + 1; l < 8; l++)
                {
                    glm::mat3x3 inPlane;
                    inPlane[0] = cubeVerts[j] - cubeVerts[i];
                    inPlane[1] = cubeVerts[k] - cubeVerts[i];
                    inPlane[2] = cubeVerts[l] - cubeVerts[i];
                    
                    if (glm::abs(glm::determinant(inPlane)) > 1e-5)
                        continue;
                    
                    vector<int> indices;
                    indices.push_back(i);
                    indices.push_back(j);
                    indices.push_back(k);
                    indices.push_back(l);
                    
                    bool found = false;
                    
                    do
                    {
                        if (glm::length(cubeVerts[indices[0]] - cubeVerts[indices[2]]) < 2.1) continue;
                        if (glm::length(cubeVerts[indices[1]] - cubeVerts[indices[3]]) < 2.1) continue;
                        
                        found = true;
                        break;
                    }
                    while (next_permutation(indices.begin(), indices.end()));
                    
                    if (glm::length(glm::cross(glm::vec3(cubeVerts[indices[1]] - cubeVerts[indices[0]]), glm::vec3(cubeVerts[indices[3]] - cubeVerts[indices[0]]))) > 4.1)
                        continue;
                    
                    assert(found);
                    
                    GLSimpleFace face;
                    face.textureId = textureId;
                    for (int t = 0; t < 4; t++)
                    {
                        face.vertices.push_back(cubeVerts[indices[t]]);
                        face.textureCoords.push_back(glm::vec2(
                            (t == 1 || t == 2) ? textureMaxX : 0,
                            (t == 2 || t == 3) ? textureMaxY : 0));
                    }
                    
                    cube.faces.push_back(face);
                }
        
    return cube;
}

void SimplePhysicsEngine::dumpRenderNoModelview(bool overlapGeometry)
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
        glVertex3f(triangle.a.x, triangle.a.y, triangle.a.z);
        glVertex3f(triangle.b.x, triangle.b.y, triangle.b.z);
        glVertex3f(triangle.c.x, triangle.c.y, triangle.c.z);
    }
    
    glEnd();
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void PhysicalBody::easyMove(MovementType type, double speed, double dt)
{
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

void SimplePhysicsEngine::processPhysics()
{
    for (PhysicalBody* body: physicalBodies)
        processBody(*body);
}

void SimplePhysicsEngine::processBody(PhysicalBody& body)
{
    for (const PhysicalTriangle& triangle: triangles)
    {
        processBodyTriangle(body, triangle);
        processBodySegment(body, triangle.a, triangle.b);
        processBodySegment(body, triangle.b, triangle.c);
        processBodySegment(body, triangle.c, triangle.a);
    }
    
    body.position += glm::vec3(0, -0.1, 0);
    
    for (const PhysicalTriangle& triangle: triangles)
        processBodyTriangle(body, triangle);
}

double absTriangleSquare(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
{
    return glm::length(glm::cross(b - a, c - a)) * 0.5;
}

bool pointInTriangle(glm::vec3 point, const PhysicalTriangle& triangle)
{
    double square = absTriangleSquare(triangle.a, triangle.b, triangle.c);
    double withPoint =
        absTriangleSquare(triangle.a, triangle.b, point) +
        absTriangleSquare(triangle.b, triangle.c, point) +
        absTriangleSquare(triangle.c, triangle.a, point);
        
    if (abs(square - withPoint) < 1e-3)
        return true;
    
    return false;
}

/*bool segmentIntersectsCircle(glm::vec3 a, glm::vec3 b, glm::vec3 center, double radius)
{
    double tProjected = glm::dot(b - a, center - a);
    if (tProjected < 0 || tProjected > glm::length2(b - a))
        return false;
    
    double distance = absTriangleSquare(a, b, center) / glm::length(b - a) * 2;
    
    return distance < radius;
}*/

void SimplePhysicsEngine::processBodySegment(PhysicalBody& body, glm::vec3 a, glm::vec3 b)
{
    double tProjected = glm::dot(b - a, body.position - a);
    if (tProjected < 0 || tProjected > glm::length2(b - a))
        return;
    
    //printf("cos alpha %lf\n", tProjected / glm::length(b - a) / glm::length(body.position - a));
    
    double distance = absTriangleSquare(a, b, body.position) / glm::length(b - a) * 2;
    
    if (distance < body.radius)
    {
        glm::vec3 projection = a + (GLfloat)(tProjected / glm::length2(b - a)) * (b - a);
        //printf("got %lf %lf\n", glm::length(body.position - projection), distance);
        assert(abs(glm::length(body.position - projection) - distance) < 1e-3);
        
        glm::vec3 fixVector = body.position - projection;
        body.position = body.position + fixVector * (GLfloat)((body.radius - distance) / body.radius);
    }
}

void SimplePhysicsEngine::processBodyTriangle(PhysicalBody& body, const PhysicalTriangle& triangle)
{
    // if projects on triangle
    //   calculate distance to plane
    //   if lower then radius
    //     apply normal correction
    
    glm::vec3 normal = glm::cross(triangle.b - triangle.a, triangle.c - triangle.a);
    normal = glm::normalize(normal);
    double A = normal.x;
    double B = normal.y;
    double C = normal.z;
    double D = -glm::dot(normal, triangle.a);
    
    double distance = glm::dot(normal, body.position) + D;
    
    if (abs(distance) < body.radius)
    {
        // check that projects onto triangle
        // or one of the sides intersects sphere
        
        glm::vec3 projection = body.position - normal * (GLfloat)distance;
        
        bool intersection = pointInTriangle(projection, triangle);
            
        if (intersection)
        {
            //printf("collision with %f %f %f\n", triangle.a.x, triangle.a.y, triangle.a.z);
            if (distance > 0)
                body.position += normal * (GLfloat)(body.radius - distance);
            else
                body.position += normal * (GLfloat)(-body.radius - distance);
        }
    }
}
