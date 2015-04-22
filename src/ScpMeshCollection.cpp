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

void ScpMeshCollection::createStaircaseMesh()
{
    beginMesh(&staircase);
    
    // transition mesh
    
    translate(0, -1, 0);
    
    setTexture("concretefloor.jpg");
    setWalkingSpeed(1.0);
    verts({ { -0.5, 0, 0 }, { 0.5, 0, 0 }, { 0.5, 0, -5 }, { -0.5, 0, -5 } });
    texCoords({ { 0, 0 }, { 1, 0 }, { 1, 5 }, { 0, 5 } });
    finishFace();
    
    pushMatrix();
    translate(0, 0, -5);
    double step = 0.27;
    for (int t = 0; t < 100; t++)
    {
        makeClimber();
        verts({ { -0.5, 0, 0 }, { 0.5, 0, 0 }, { 0.5, step, 0 }, { -0.5, step, 0 } });
            texCoords({ { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } });
        translate(0, step, 0);
        finishFace();
        
        setWalkingSpeed(0.6);
        verts({ { -0.5, 0, 0 }, { 0.5, 0, 0 }, { 0.5, 0, -step }, { -0.5, 0, -step } });
        texCoords({ { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } });
        translate(0, 0, -step);
        finishFace();
    }
    popMatrix();
    
    /*setTexture("brickwall.jpg");
    verts({ { -0.5, 0.1, 0 }, { 0.5, 0.1, 0 }, { 0.5, 0.1, -5 }, { -0.5, 0.1, -5 } });
    texCoords({ { 0, 0 }, { 1, 0 }, { 1, 5 }, { 0, 5 } });
    finishFace();*/
    
    finishMesh();
}
