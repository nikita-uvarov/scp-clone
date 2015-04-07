#include "ScpMeshCollection.h"

#include <vector>
#include <algorithm>

using namespace std;

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
                    face.isClimber = false;
                    for (int t = 0; t < 4; t++)
                    {
                        face.vertices.push_back(cubeVerts[indices[t]]);
                        face.textureCoords.push_back(glm::vec2(
                            (t == 1 || t == 2) ? textureMaxX : 0,
                            (t == 2 || t == 3) ? textureMaxY : 0));
                    }
                    
                    cube.faces.push_back(face);
                }
    
    cube.decomposeIntoSingleTextureMeshes();
    return cube;
}

GLTexture& SimpleTextureManager::get(string name)
{
    name = "resources/" + name;
    
    auto found = textureByName.find(name);
    if (found != textureByName.end())
        return *found->second;
    
    textureByName[name].reset(new GLTexture(loadTexture(name.c_str())));
    return *textureByName[name];
}

void ScpMeshCollection::loadMeshes()
{
    GLTexture& cubeTexture = textureManager.get("verticals.jpg");
    cubeMesh = createCubeMesh(cubeTexture.openglId, cubeTexture.getMaxU(), cubeTexture.getMaxV());
    
    createStaircaseMesh();
}

void StateMachineMeshEditor::addQuad(glm::vec3 a, glm::vec3 b, double textureScale)
{
    // x1 y1 z  x2 y2 z
    // x1 y z1 x2 y z2 - swapYZ
    // x y1 z1 x y2 z2 - swapXZ
    
    bool swapYZ = false, swapXZ = false;
    
    if (abs(a.z - b.z) < 1e-3)
    {
    }
    else if (abs(a.y - b.y) < 1e-3)
    {
        swapYZ = true;
        swap(a.y, a.z);
        swap(b.y, b.z);
    }
    else if (abs(a.x - b.x) < 1e-3)
    {
        swapXZ = true;
        swap(a.x, a.z);
        swap(b.x, b.z);
    }
    else SDL_assert(!"added quad should be parallel to XY, XZ or YZ planes");
    
    SDL_assert(currentFace.vertices.empty());
    
    // now x & y are different and z is common
    
    currentFace.vertices.insert(currentFace.vertices.end(), { { a.x, a.y, a.z }, { b.x, a.y, a.z }, { b.x, b.y, a.z }, { a.x, b.y, a.z } });
    
    bool smartWrapping = abs(textureScale) > 1e-3;
    
    if (!smartWrapping)
        texCoords({ { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } });
    else
    {
        SDL_assert(abs(currentTexture.getMaxU() - 1) < 1e-3);
        SDL_assert(abs(currentTexture.getMaxV() - 1) < 1e-3);
        
        /*texCoords({
            { (a.x + a.z) / textureScale, a.y / textureScale },
            { (b.x + a.z) / textureScale, a.y / textureScale },
            { (b.x + a.z) / textureScale, b.y / textureScale },
            { (a.x + a.z) / textureScale, b.y / textureScale } });*/
        
        // TODO
    }
    
    for (glm::vec3& vertex: currentFace.vertices)
    {
        if (swapYZ)
            swap(vertex.y, vertex.z);
        if (swapXZ)
            swap(vertex.x, vertex.z);
        
        vertex = glm::vec3(transformStack.back() * glm::vec4(vertex, 1.0));
        
        glm::vec3 reverseTransform = vertex;
        
        if (swapYZ)
            swap(reverseTransform.y, reverseTransform.z);
        if (swapXZ)
            swap(reverseTransform.x, reverseTransform.z);
        
        if (smartWrapping)
            texCoords({ { (reverseTransform.x + reverseTransform.z) / textureScale, reverseTransform.y / textureScale } });
    }
    
    finishFace();
}

void ScpMeshCollection::createStaircaseMesh()
{
    const char* brick = "brickwall.jpg";
    const char* floor = "concretefloor.jpg";
    
    beginMesh(&staircase);
    
    // transition mesh
    
    //translate(0, -1, 0);
    
    // first tunnel - walls
    setTexture(brick);
    addQuad( { -0.5, 0, 0 }, { -0.5, 3, -5  }, 2.0);
    addQuad( { 0.5, 0, 0 }, { 0.5, 3, -5  }, 2.0);
    
    // first tunnel - floor & ceiling
    setTexture(floor);
    setWalkingSpeed(1.0);
    addQuad( { -0.5, 0, 0 }, { 0.5, 0, -5 }, 2.0);
    setWalkingSpeed(1.0);
    addQuad( { -0.5, 3, 0 }, { 0.5, 3, -5 }, 2.0);
    
    // staircase
    translate(0, 0, -5);
    pushMatrix();
    double step = 0.27;
    for (int t = 0; t < 15; t++)
    {
        makeClimber();
        addQuad( { -0.5, 0, 0 }, { 0.5, -step, 0 }, 2.0);
        translate(0, -step, 0);
        
        setWalkingSpeed(0.6);
        addQuad( { -0.5, 0, 0 }, { 0.5, 0, -step }, 2.0);
        translate(0, 0, -step);
    }
    popMatrix();
    
    // staircase ceiling
    verts({ { -0.5, 3, 0 }, { 0.5, 3, 0 }, { 0.5, 3 - step * 15, -step * 15 }, { -0.5, 3 - step * 15, -step * 15 } });
    texCoords({ { 0, 0 }, { 1, 0 }, { 1, step * 15 / 2 * sqrt(2) }, { 0, step * 15 / 2 * sqrt(2) } });
    finishFace();
    
    // staircase walls
    setTexture(brick);
    addQuad( { -0.5, 3, 0 }, { -0.5, -step * 15, -step * 15 }, 2.0);
    addQuad( { 0.5, 3, 0 }, { 0.5, -step * 15, -step * 15 }, 2.0);
    
    translate(0, -step * 15, -step * 15);
    
    // after-staircase tunnel - walls
    addQuad( { -0.5, 0, 0 }, { -0.5, 3, -3  }, 2.0);
    addQuad( { 0.5, 0, 0 }, { 0.5, 3, -2  }, 2.0);
    
    // after-staircase tunnel - floor & ceiling
    setTexture(floor);
    setWalkingSpeed(1.0);
    addQuad( { -0.5, 0, 0 }, { 0.5, 0, -2 }, 2.0);
    setWalkingSpeed(1.0);
    addQuad( { -0.5, 3, 0 }, { 0.5, 3, -2 }, 2.0);
    
    translate(0, 0, -2);
    translate(-0.5, 0, -0.5);
    rotate(-M_PI / 2, glm::vec3(0, 1, 0));
    
    // after-staircase rotated tunnel - walls
    
    setTexture(brick);
    
    addQuad( { -0.5, 0, 0 }, { -0.5, 3, -3 }, 2.0 );
    addQuad( { 0.5, 0, -1 }, { 0.5, 3, -3 }, 2.0 );
    
    // after-staircase rotated tunnel - floor & ceiling
    setTexture(floor);
    
    addQuad( { -0.5, 0, 0 }, { 0.5, 0, -3 }, 2.0 );
    addQuad( { -0.5, 3, 0 }, { 0.5, 3, -3 }, 2.0 );
    
    translate(0, 0, -3);
    markCurrentPosition("continuation-point");
    
    finishMesh();
}
