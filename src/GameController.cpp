#include "GameController.h"

#include <iostream>
#include <vector>
#include <algorithm>

using namespace std;

void GameController::initializeGraphics(int width, int height)
{
    texture = loadTexture("resources/brickwall.jpg");
    cube = createCubeMesh(texture.openglId, texture.getMaxU(), texture.getMaxV());
    cube.decomposeIntoSingleTextureMeshes();
    
    for (int z = 5; z >= -20; z -= 3)
    {
        cubePositions.push_back(glm::vec3(-3, 0, z));
        cubePositions.push_back(glm::vec3(3, 0, z));
    }
    
    physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-5, -1, -5), glm::vec3(5, -1, -5), glm::vec3(5, -1, 5) });
    physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-5, -1, -5), glm::vec3(5, -1, 5), glm::vec3(-5, -1, 5) });
    
    GLfloat step = 0.2;
    for (int t = 0; t < 10; t++)
    {
        physicsEngine.triangles.push_back(PhysicalTriangle {
            glm::vec3(-5, -1 - step * t, -5 - step * t),
            glm::vec3(5, -1 - step * t, -5 - step * t),
            glm::vec3(5, -1 - step * (t * 1), -5 - step * t) });
    }
    
    //physicsEngine.triangles.push_back(PhysicalTriangle { glm::vec3(-1, 0, -1), glm::vec3(1, -1, 1), glm::vec3(-1, 0, 1) });
    
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
    
    currentWidth = width;
    currentHeight = height;
    
    cameraVector = glm::vec3(0, 0, -1);
    player.position = glm::vec3(0, 0, +1);
    player.radius = 0.25;
    updatePlayerDirection();
    
    physicsEngine.physicalBodies.push_back(&player);

	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
    
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
    
	glMatrixMode(GL_MODELVIEW);
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
    
    double speed = 1.0;
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
    
    physicsEngine.processPhysics();
}

void GameController::keyPressed(SDL_Keycode keycode)
{
    pressedKeys.insert(keycode);
}

void GameController::keyReleased(SDL_Keycode keycode)
{
    pressedKeys.erase(keycode);
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
    
    glm::vec3 playerHeightVector(0, 1 - player.radius, 0);
    
    // make vertical FOV fixed
    float aspectRatio = (float)currentWidth / (float)currentHeight;
    
    //projectionMatrix = glm::frustum(-1.0 * aspectRatio, 1.0 * aspectRatio, -1.0, 1.0, 1.0, 1e3);
    projectionMatrix = glm::perspective(45.0f / 180.0f * (GLfloat)M_PI, aspectRatio, 0.1f, 1e3f);
    viewMatrix = glm::lookAt(player.position + playerHeightVector,
                             player.position + playerHeightVector + cameraVector, glm::vec3(0, 1, 0));
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, -5));
    modelMatrix = glm::rotate(modelMatrix, angle, glm::vec3(sin(angle), sin(angle + 2 * M_PI / 3), sin(angle + M_PI / 3)));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(2, 2, 2));
    
#if 0
    glFogi(GL_FOG_MODE, GL_EXP);
    glFogfv(GL_FOG_COLOR, glm::value_ptr(glm::vec3(0, 0, 0)));
    glFogf(GL_FOG_DENSITY, 0.15f);
    glHint(GL_FOG_HINT, GL_DONT_CARE);
    glFogf(GL_FOG_START, 1.0f);
    glFogf(GL_FOG_END, 5.0f);
    glEnable(GL_FOG);
#endif
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture.openglId);
    
    for (auto pos: cubePositions)
    {
        glm::mat4 modelMatrix;
        modelMatrix = glm::translate(modelMatrix, pos);
        
        glm::mat4 finalMatrix = projectionMatrix * viewMatrix * modelMatrix;
        
        glLoadMatrixf(glm::value_ptr(finalMatrix));
        cube.render();
    }
    
    glm::mat4 finalMatrix = projectionMatrix * viewMatrix;
    glLoadMatrixf(glm::value_ptr(finalMatrix));
    
    physicsEngine.dumpRenderNoModelview(false);
}
