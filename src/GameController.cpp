#include "GameController.h"

#include <glm/gtx/vector_angle.hpp>

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

GameController::GameController(): meshCollection(textureManager) {}

// gets 'shortest rotation' (without 'spinning')
glm::mat4 getRotationMatrix(glm::vec3 from, glm::vec3 to)
{
    glm::mat4 rotation;
    
    glm::vec3 axis = glm::cross(from, to);
    if (glm::length(axis) < 1e-3)
        return rotation;
    
    double angle = glm::orientedAngle(from, to, axis);
    rotation = glm::rotate(rotation, (GLfloat)angle, axis);
    return rotation;
}

void GameController::initializeGraphics(int width, int height)
{
    firstPersonMode = false;
    wireframeMode = false;
    physicsDebugMode = true;
    fogEnabled = false;
    
    meshCollection.loadMeshes();
    
    //texture = loadTexture("resources/verticals.jpg");
    //cube = createCubeMesh(texture.openglId, texture.getMaxU(), texture.getMaxV());
    //cube.decomposeIntoSingleTextureMeshes();
    
    for (int z = 5; z >= -20; z -= 3)
    {
        for (int t = 0; t < 2; t++)
            worldContainer.addPositionedMesh(meshCollection.cubeMesh, glm::vec3((t * 2 - 1) * 3, 0, z));
        
        //cubePositions.push_back(glm::vec3(-3, 0, z));
        //cubePositions.push_back(glm::vec3(3, 0, z));
    }
    
    glm::mat4 currentModelview;
    glm::vec3 viewDirection(0, 0, -1);
    
    for (int i = 0; i < 10; i++)
    {
        GLSimpleMesh& simpleMesh = meshCollection.staircase;
        
        GLPositionedMesh positioned;
        positioned.baseMesh = &simpleMesh;
        positioned.modelMatrix = currentModelview;
        
        worldContainer.positionedMeshes.push_back(positioned);
        
        MeshMarker continuation = simpleMesh.getObligatoryMarker("continuation-point");
        
        currentModelview = glm::translate(currentModelview, continuation.position);
        currentModelview = currentModelview * getRotationMatrix(viewDirection, continuation.direction);
    }
    
    //physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-5, -1, -5), glm::vec3(5, -1, -5), glm::vec3(5, -1, 5), 1 });
    //physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-5, -1, -5), glm::vec3(5, -1, 5), glm::vec3(-5, -1, 5), 1 });
    
    GLfloat step = 0.27f;
    GLfloat yOffset = 0, zOffset = 0;
    GLfloat horizSlope = 0.75f;
    horizSlope = 0;
    
#if 0
    int from = (int)physicsEngine.triangles.size();
    
    for (int t = 0; t < 100; t++)
    {
#define vertex(x, y, z) glm::vec3((x), -1 + (y) + yOffset, -5 + (z) + zOffset)
#define triangle(a, b, c) physicsEngine.triangles.push_back(PhysicalTriangle { a, b, c, 0.6 })
#define quad(a, b, c, d) { triangle(a, b, c); triangle(a, c, d); }
        
        quad(vertex(0, 0, 0), vertex(0, +step, -horizSlope), vertex(1, +step, -horizSlope), vertex(1, 0, 0));
        
        yOffset += +step;
        zOffset += -horizSlope;
        
        quad(vertex(0, 0, 0), vertex(1, 0, 0), vertex(1, 0, -step), vertex(0, 0, -step));
        
        zOffset += -step;
        
        /*physicsEngine.triangles.push_back(PhysicalTriangle {
            glm::vec3(-5, -1 - step * t, -5 - step * t),
            glm::vec3(5, -1 - step * t, -5 - step * t),
            glm::vec3(5, -1 - step * (t * 1), -5 - step * t) });*/
        
#undef vertex
#undef triangle
#undef quad
    }
    
    int to = (int)physicsEngine.triangles.size();
    
#endif
    
#if 0
    int nClimbable = 0;
    for (int i = from; i < to; i++)
    {
        auto& triangle = physicsEngine.triangles[i];
        GLfloat yMin = min(triangle.a.y, min(triangle.b.y, triangle.c.y));
        GLfloat yMax = max(triangle.a.y, max(triangle.b.y, triangle.c.y));
        
        if (yMax - yMin > 1e-3)
        {
            //triangle.walkSpeed = 1;
            //triangle.climbSpeed = 10;
            triangle.isClimber = true;
            nClimbable++;
            //printf("Ok climbable\n");
        }
    }
    printf("out of %d %d climbable\n", to - from, nClimbable);
#endif
    
    //physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-1, 0, -1), glm::vec3(1, -1, 1), glm::vec3(-1, 0, 1) });
    
#if 0
    for (glm::vec3 cubePosition: cubePositions)
        for (GLSingleTextureMesh subMesh: cube.singleTextureMeshDecomposition)
        {
            SDL_assert(subMesh.triangleIndices.size() % 3 == 0);
            
            for (int triangleIndex = 0; triangleIndex * 3 < (int)subMesh.triangleIndices.size(); triangleIndex++)
            {
                vector<glm::vec3> vertices;
                for (int t = 0; t < 3; t++)
                    vertices.push_back(cubePosition + subMesh.vertices[subMesh.triangleIndices[triangleIndex * 3 + t]]);
                
                PhysicalTriangle triangle = { vertices[0], vertices[1], vertices[2] };
                physicsEngine.triangles.push_back(triangle);
            }
        }
#endif
    
    currentWidth = width;
    currentHeight = height;
    
    cameraVector = glm::vec3(0, 0, -1);
    player.position = glm::vec3(0, 0.5, 0);
    player.radius = 0.2;
    //player.radius = 0.05;
    player.currentWalkSpeed = 1;
    updatePlayerDirection();
    
    reloadShaders();
    
    //physicsEngine.physicalBodies.push_back(&player);

	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
    
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
    
	glMatrixMode(GL_MODELVIEW);
}

void GameController::reloadShaders()
{
    if (vertexShader)
    {
        glDeleteShader(vertexShader);
        vertexShader = 0;
    }
    
    if (fragmentShader)
    {
        glDeleteShader(fragmentShader);
        fragmentShader = 0;
    }
    
    if (shaderProgram)
    {
        glUseProgram(0);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    
    string defineString = "";
    for (auto& it: shaderDefines)
        defineString += "#define " + it + "\n";
    
    vertexShader = loadGlShader("resources/vertex-shader.glsl", GL_VERTEX_SHADER, defineString.c_str());
    fragmentShader = loadGlShader("resources/fragment-shader.glsl", GL_FRAGMENT_SHADER, defineString.c_str());
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    GLint linkOk;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkOk);
    SDL_assert(linkOk);
    
    glUseProgram(shaderProgram);
}

void GameController::updatePlayerDirection()
{
    player.direction = glm::normalize(glm::vec3(cameraVector.x, 0, cameraVector.z));
}

void GameController::changeWindowSize(int newWidth, int newHeight)
{
	glViewport(0, 0, newWidth, newHeight);
    
    currentWidth = newWidth;
    currentHeight = newHeight;
}

void GameController::simulateWorld(double msPassed)
{
    currentTime += msPassed / 1000.0;
    
    double speed = 1;
    double dt = msPassed / 1000.0;
 
    updatePlayerDirection();
    
    if (pressedKeys.count(SDLK_a))
        player.easyMove(MovementType::STRAFE_LEFT, speed, dt);
    
    if (pressedKeys.count(SDLK_d))
        player.easyMove(MovementType::STRAFE_RIGHT, speed, dt);
    
    if (pressedKeys.count(SDLK_w))
        player.easyMove(MovementType::FORWARD, speed, dt);
    
    if (pressedKeys.count(SDLK_s))
        player.easyMove(MovementType::BACKWARD, speed, dt);
    
    if (pressedKeys.count(SDLK_SPACE))
    {
        player.yAccel = 0.1;
        //player.position += glm::vec3(0, 1, 0);
    }
    
    player.currentWalkSpeed = 1;
    worldContainer.processPhysics(player);
    //physicsEngine.processPhysics();
}

void GameController::keyPressed(SDL_Keycode keycode)
{
    pressedKeys.insert(keycode);
}

void GameController::keyReleased(SDL_Keycode keycode)
{
    pressedKeys.erase(keycode);
    
    if (keycode == SDLK_q)
        firstPersonMode = !firstPersonMode;
    
    if (keycode == SDLK_o)
        wireframeMode = !wireframeMode;
    
    if (keycode == SDLK_p)
        physicsDebugMode = !physicsDebugMode;
    
    if (keycode == SDLK_f)
    {
        fogEnabled = !fogEnabled;
        
        if (fogEnabled)
            shaderDefines.insert("FOG");
        else
            shaderDefines.erase("FOG");
        
        reloadShaders();
    }
}

void GameController::relativeMouseMotion(int dx, int dy)
{
    GLfloat rdx = (GLfloat)dx / 10000.0f, rdy = (GLfloat)dy / 10000.0f;
    
    glm::mat4 rotation;
    rotation = glm::rotate(rotation, rdx, glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, rdy, glm::cross(cameraVector, glm::vec3(0, 1, 0)));
    
    cameraVector = glm::normalize(cameraVector * glm::mat3(rotation));
}

void GameController::renderFrame()
{   
    GLfloat angle = (GLfloat)currentTime / 5;
    
    projectionMatrix = glm::mat4();
    viewMatrix = glm::mat4();
    modelMatrix = glm::mat4();
    
    glm::vec3 playerHeightVector(0, 2 - player.radius, 0);
    
    // make vertical FOV fixed
    float aspectRatio = (float)currentWidth / (float)currentHeight;
    
    //projectionMatrix = glm::frustum(-1.0 * aspectRatio, 1.0 * aspectRatio, -1.0, 1.0, 1.0, 1e3);
    projectionMatrix = glm::perspective(45.0f / 180.0f * (GLfloat)M_PI, aspectRatio, 0.1f, 1e3f);
    
    if (!firstPersonMode)
    {
        viewMatrix = glm::lookAt(player.position + playerHeightVector,
                                player.position + playerHeightVector + cameraVector, glm::vec3(0, 1, 0));
    }
    else
    {
        viewMatrix = glm::lookAt(player.position + playerHeightVector - cameraVector * 4.0f,
                                player.position + playerHeightVector, glm::vec3(0, 1, 0));
    }
    
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, -5));
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(sin(angle), sin(angle + 2 * M_PI / 3), sin(angle + M_PI / 3)));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(2, 2, 2));
    
    /*if (fogEnabled)
    {
        glFogi(GL_FOG_MODE, GL_LINEAR);
        glFogfv(GL_FOG_COLOR, glm::value_ptr(glm::vec3(0, 0, 0)));
        glFogf(GL_FOG_DENSITY, 1.0f);
        glHint(GL_FOG_HINT, GL_DONT_CARE);
        glFogf(GL_FOG_START, 1.5f);
        glFogf(GL_FOG_END, 3.0f);
        glEnable(GL_FOG);
    }
    else
    {
        glDisable(GL_FOG);
    }*/
    
    if (wireframeMode)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else    
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_COLOR_MATERIAL);
    //glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, texture.openglId);
    
    /*for (auto pos: cubePositions)
    {
        glm::mat4 modelMatrix;
        modelMatrix = glm::translate(modelMatrix, pos);
        
        glm::mat4 finalMatrix = projectionMatrix * viewMatrix * modelMatrix;
        
        glLoadMatrixf(glm::value_ptr(finalMatrix));
        cube.render();
    }*/
    
    worldContainer.renderWorld(projectionMatrix * viewMatrix);
    
    glm::mat4 finalMatrix = projectionMatrix * viewMatrix;
    glLoadMatrixf(glm::value_ptr(finalMatrix));
    
    //if (physicsDebugMode)
    {
        //physicsEngine.dumpRenderNoModelview(false, physicsDebugMode);
        //worldContainer.dumpRenderPhysics(player);
    }

    if (physicsDebugMode)
    {
        modelMatrix = glm::mat4();
        modelMatrix = glm::translate(modelMatrix, player.position);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(player.radius, player.radius, player.radius));
        
        glLoadMatrixf(glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
        meshCollection.cubeMesh.render();
    }
}
