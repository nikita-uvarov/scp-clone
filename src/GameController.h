#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "Common.h"
#include "IGameController.h"
#include "GLUtils.h"
#include "ScpMeshCollection.h"
#include "ShaderUtils.h"

#include <set>

namespace sge
{

class FullScreenRenderTarget
{
public :
    GLuint textureId;
    GLuint zBufferRbo;
    GLuint fbo;
    
    void create(int width, int height);
    void destroy();
    
    void renderScreenQuad();
};

class GameController : public IGameController
{
    int currentWidth, currentHeight;
    ftype currentTime;
    
    mat4 projectionMatrix, viewMatrix, modelMatrix;
    
    std::set<SDL_Keycode> pressedKeys;
    
    bool firstPersonMode;
    bool wireframeMode;
    bool fogEnabled;
    bool physicsDebugMode;
    bool enableSimpleBlur;
    
    CharacterController player;
    vec3 cameraVector;
    
    std::vector<vec3> cubePositions;
    
    //SimplePhysicsEngine physicsEngine;
    
    SimpleWorldContainer worldContainer;
    
    ScpMeshCollection meshCollection;
    GLSimpleMesh loadedMesh;
    
    std::set<std::string> shaderDefines;
    
    GLuint vertexShader, fragmentShader;
    GLuint shaderProgram;
    
    FullScreenRenderTarget blurBufferA, blurBufferB;
    
    void reloadShaders();
    
    void updatePlayerDirection();
    
public :
    
    void initializeGraphics(int width, int height);
    
    void changeWindowSize(int newWidth, int newHeight);
    
    void renderFrame();
    
    void simulateWorld(ftype msPassed);
    
    void keyPressed(SDL_Keycode keycode);
    void keyReleased(SDL_Keycode keycode);
    void relativeMouseMotion(int dx, int dy);
};

}

#endif // GAME_CONTROLLER_H
