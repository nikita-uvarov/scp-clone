#include "ScpMeshCollection.h"

#include <vector>
#include <algorithm>
#include <sstream>

#include <pugixml.hpp>

using namespace std;
using namespace sge;

GLSimpleMesh createCubeMesh(GLuint textureId, ftype textureMaxX, ftype textureMaxY)
{
    GLSimpleMesh cube;
    
    vector<vec3> cubeVerts;
    
    for (int x = -1; x <= 1; x += 2)
        for (int y = -1; y <= 1; y += 2)
            for (int z = -1; z <= 1; z += 2)
                cubeVerts.push_back(vec3(x, y, z));
            
    assert(cubeVerts.size() == 8);
    
    for (int i = 0; i < 8; i++)
        for (int j = i + 1; j < 8; j++)
            for (int k = j + 1; k < 8; k++)
                for (int l = k + 1; l < 8; l++)
                {
                    mat3 inPlane;
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
                    
                    if (glm::length(glm::cross(vec3(cubeVerts[indices[1]] - cubeVerts[indices[0]]), vec3(cubeVerts[indices[3]] - cubeVerts[indices[0]]))) > 4.1)
                        continue;
                    
                    assert(found);
                    
                    GLSimpleFace face;
                    face.textureId = textureId;
                    face.isClimber = false;
                    face.isCollisionActive = true;
                    face.walkingSpeed = 1.0;
                    for (int t = 0; t < 4; t++)
                    {
                        face.vertices.push_back(cubeVerts[indices[t]]);
                        
                        face.textureCoords.push_back(glm::vec2(
                            (t == 1 || t == 2) ? 1 - textureMaxX : 1,
                            (t == 2 || t == 3) ? 1 - textureMaxY : 1));
                    }
                    
                    cube.faces.push_back(face);
                }
    
    cube.decomposeIntoSingleTextureMeshes();
    return cube;
}

void ScpMeshCollection::loadMeshes()
{
    const Texture& cubeTexture = TextureManager::instance().retrieveTexture("verticals.jpg");
    cubeMesh = createCubeMesh(cubeTexture.openglId, cubeTexture.getMaxU(), cubeTexture.getMaxV());
    
    createStaircaseMesh();
    createPlaneMesh();
}

void StateMachineMeshEditor::addQuad(vec3 a, vec3 b, ftype textureScale)
{
    // x1 y1 z  x2 y2 z
    // x1 y z1  x2 y z2 - swapYZ
    // x y1 z1  x y2 z2 - swapXZ
    
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
    else critical_error("%s: added quad should be parallel to XY, XZ or YZ planes", __FUNCTION__);
    
    SDL_assert(currentFace.vertices.empty());
    
    // now x & y are different and z is common
    
    currentFace.vertices.insert(currentFace.vertices.end(), { { a.x, a.y, a.z }, { b.x, a.y, a.z }, { b.x, b.y, a.z }, { a.x, b.y, a.z } });
    
    bool smartWrapping = abs(textureScale) > FTYPE_WEAK_EPS;
    
    if (!smartWrapping)
        texCoords({ { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } });
    else
    {
        SDL_assert(weakEq(currentTexture->getMaxU(), 1));
        SDL_assert(weakEq(currentTexture->getMaxV(), 1));
    }
    
    for (vec3& vertex: currentFace.vertices)
    {
        if (swapYZ)
            swap(vertex.y, vertex.z);
        if (swapXZ)
            swap(vertex.x, vertex.z);
        
        vertex = vec3(transformStack.back() * vec4(vertex, 1.0));
        
        vec3 reverseTransform = vertex;
        
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
    ftype step = 0.27;
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
    rotate(-M_PI / 2, vec3(0, 1, 0));
    
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

GLSimpleMesh ScpMeshCollection::createStaircase(ftype horizOne, ftype vertOne, ftype horizTwo, ftype vertTwo, int nSteps)
{
    GLSimpleMesh stairs;
    
    beginMesh(&stairs);
    
    for (int t = 0; t < nSteps; t++)
    {
        setWalkingSpeed(0.6);
        addQuad( { -0.5, 0, 0 }, { 0.5, vertOne, -horizOne }, 2.0);
        translate(0, vertOne, -horizOne);
        
        makeClimber();
        addQuad( { -0.5, 0, 0 }, { 0.5, vertTwo, -horizTwo }, 2.0);
        translate(0, vertTwo, -horizTwo);
    }
    
    finishMesh();
    
    return stairs;
}

void ScpMeshCollection::createPlaneMesh()
{
    beginMesh(&plane);
    
    setTexture("concretefloor.jpg");
    
    addQuad({ -5, 0, -5 }, { 5, 0, 5 }, 1.0);
    
    finishMesh();
}

template<class T>
vector<T> splitStringIntoData(string str)
{
    vector<T> dataVector;
    istringstream inputStream(str);
    
    T data;
    while (inputStream >> data)
        dataVector.push_back(data);
    
    return dataVector;
}

int parseInt(string str)
{
    vector<int> data = splitStringIntoData<int>(str);
    SDL_assert(data.size() == 1);
    
    return data[0];
}

string extractUrlPath(string url)
{
    SDL_assert(url[0] == '#');
    return url.substr(1, url.size() - 1);
}

GLSimpleMesh sge::loadColladaMesh(const char* colladaFileName)
{
    using namespace pugi;
    
    GLSimpleMesh mesh = GLSimpleMesh();
    
    xml_document document;
    xml_parse_result parseResult = document.load_file(colladaFileName);
    
    if (!parseResult)
        critical_error("Failed to parse XML file '%s': %s\n", colladaFileName, parseResult.description());
    
    SDL_assert(parseResult);
    
    // the following libraries need to be briefly parsed:
    // images - effects - materials - 
    
    // parse 'library_images' extracting paths from image descriptions
    
    map<string, string> imageToPath;
    
    xml_node libraryImages = document.child("COLLADA").child("library_images");
    SDL_assert(libraryImages);
    
    for (xml_node image = libraryImages.child("image"); image; image = image.next_sibling("image"))
    {
        SDL_assert(string(image.attribute("name").value()) == image.attribute("id").value());
        imageToPath[image.attribute("id").value()] = image.child("init_from").child_value();
    }
    
    // parse 'library_effects' extracting images from effect descriptions
    
    map<string, string> effectToImage;
    
    xml_node libraryEffects = document.child("COLLADA").child("library_effects");
    SDL_assert(libraryEffects);
    
    struct nameMatches
    {
        string name;
        
        nameMatches(string name): name(name) {}
        
        bool operator()(pugi::xml_attribute attr) const
        {
            return (attr.name() == name);
        }

        bool operator()(pugi::xml_node node) const
        {
            return node.name() == name;
        }
    };
    
    for (xml_node effect = libraryEffects.child("effect"); effect; effect = effect.next_sibling("effect"))
    {
        xml_node sampleSource = effect.find_node(nameMatches("init_from"));
        SDL_assert(sampleSource);
        
        string effectId = effect.attribute("id").value();
        string imageId = sampleSource.child_value();
        
        if (imageToPath.find(imageId) == imageToPath.end())
            critical_error("Failed to resolve Collada image reference: '%s'", imageId.c_str());
        
        effectToImage[effectId] = imageId;
    }
    
    // parse 'library_materials'
    
    map<string, string> materialToEffect;
    
    xml_node libraryMaterials = document.child("COLLADA").child("library_materials");
    
    for (xml_node material = libraryMaterials.child("material"); material; material = material.next_sibling("material"))
    {
        string effectUrl = material.child("instance_effect").attribute("url").value();
        effectUrl = extractUrlPath(effectUrl);
        
        materialToEffect[material.attribute("id").value()] = effectUrl;
    }
    
    // parse 'library_geometries'
    
    xml_node libraryGeometries = document.child("COLLADA").child("library_geometries");
    xml_node geometryNode = libraryGeometries.child("geometry");
    SDL_assert(geometryNode && !geometryNode.next_sibling());
    
    xml_node meshNode = geometryNode.child("mesh");
    SDL_assert(meshNode && !meshNode.next_sibling());
    
    map<string, vector<ftype>> sourceArrayByName;
    
    // process 'source' entries (different vertex attributes)
    for (xml_node source = meshNode.child("source"); source; source = source.next_sibling("source"))
    {
        string id = source.attribute("id").value();
        
        xml_node floatArray = source.child("float_array");
        int nElements = parseInt(floatArray.attribute("count").value());
        string elementsString = floatArray.child_value();
        
        vector<ftype> elements = splitStringIntoData<ftype>(elementsString);
        assert(elements.size() == nElements);
        
        sourceArrayByName[id] = elements;
    }
    
    // process 'vertices' entry
    xml_node vertices = meshNode.child("vertices");
    SDL_assert(vertices && !vertices.next_sibling("vertices"));
    
    string verticesSource = extractUrlPath(vertices.child("input").attribute("source").value());
    SDL_assert(sourceArrayByName.find(verticesSource) != sourceArrayByName.end());
    
    vector<ftype>& verticesSourceArray = sourceArrayByName[verticesSource];
    
    // process 'polylist' entries
    for (xml_node polylist = meshNode.child("polylist"); polylist; polylist = polylist.next_sibling("polylist"))
    {
        string material = polylist.attribute("material").value();
        int nFaces = parseInt(polylist.attribute("count").value());
        
        vector<int> indices = splitStringIntoData<int>(polylist.child("p").child_value());
        vector<int> vertexCounts = splitStringIntoData<int>(polylist.child("vcount").child_value());
        
        //printf("%d vertex counts, %d faces\n", (int)vertexCounts.size(), nFaces);
        //fflush(stdout);
        
        SDL_assert(vertexCounts.size() == nFaces);
        SDL_assert(indices.size() % (nFaces * vertexCounts[0]) == 0);
        
        vector<ftype>* texcoordsFrom = nullptr;
        int texcoordOffset = 0;
        
        for (xml_node input = polylist.child("input"); input; input = input.next_sibling("input"))
        {
            string semantic = input.attribute("semantic").value();
            string source = extractUrlPath(input.attribute("source").value());
            int offset = parseInt(input.attribute("offset").value());
            
            if (semantic == "TEXCOORD")
            {
                SDL_assert(!texcoordsFrom);
                SDL_assert(sourceArrayByName.find(source) != sourceArrayByName.end());
                texcoordOffset = offset;
                texcoordsFrom = &sourceArrayByName[source];
            }
        }
        
        SDL_assert(texcoordsFrom);
        
        printf("material '%s' effect '%s' image '%s' path '%s'\n",
               material.c_str(),
               materialToEffect[material].c_str(),
               effectToImage[materialToEffect[material]].c_str(),
               imageToPath[effectToImage[materialToEffect[material]]].c_str());
        
        string texturePath = imageToPath[effectToImage[materialToEffect[material]]];
        
        static string lastOkTexture = "";
        
        if (texturePath.empty())
            texturePath = lastOkTexture;
        else
            lastOkTexture = texturePath;
        
        GLSingleTextureMesh submesh;
        
        GLuint currentTexture = TextureManager::instance().retrieveTexture(texturePath).openglId;
        submesh.textureId = currentTexture;
        
        int perVertex = (int)indices.size() / (nFaces * vertexCounts[0]);
        int currentOffset = 0;
        for (int i = 0; i < nFaces; i++)
        {
            GLSimpleFace face;
            face.textureId = currentTexture;
            face.isCollisionActive = false;
            face.isClimber = false;
            face.walkingSpeed = 1.0;
            
            for (int j = 0; j < vertexCounts[i]; j++)
            {
                // FIXME: due to mesh organisation,
                // separate texture coords indices are not supported, so exactly 3 vertices
                // per triangle are added
                ftype* base = &verticesSourceArray[indices[currentOffset] * 3];
                face.vertices.push_back(vec3(*base, *(base + 2), -*(base + 1)));
                
                ftype* textureBase = texcoordsFrom->data() + indices[currentOffset + texcoordOffset] * 2;
                face.textureCoords.push_back(glm::vec2(*textureBase, *(textureBase + 1)));
                
                submesh.vertices.push_back(face.vertices.back());
                submesh.textureCoords.push_back(face.textureCoords.back());
                submesh.triangleIndices.push_back((int)submesh.vertices.size() - 1);
                
                currentOffset += perVertex;
            }
            
            //printf("pushed face %d\n", vertexCounts[i]);
            mesh.faces.push_back(face);
        }
        
        mesh.singleTextureMeshDecomposition.push_back(submesh);
    }
    
    // dump parsed info
    
    for (auto it: imageToPath)
        printf("image '%s' path '%s'\n", it.first.c_str(), it.second.c_str());
    
    for (auto it: effectToImage)
        printf("effect '%s' image '%s'\n", it.first.c_str(), it.second.c_str());
    
    for (auto it: materialToEffect)
        printf("material '%s' effect '%s'\n", it.first.c_str(), it.second.c_str());
    
    //printf("Decomposing into single texture meshes...\n");
    //fflush(stdout);
    
    //mesh.decomposeIntoSingleTextureMeshes();
    //printf("Done...\n");
    //fflush(stdout);
    
    return mesh;
}
