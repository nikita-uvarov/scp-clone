#ifndef SGE_GAME_CONTROLLER_H
#define SGE_GAME_CONTROLLER_H

#include "Common.h"
#include "IGameController.h"
#include "GLUtils.h"
#include "ScpMeshCollection.h"
#include "ShaderUtils.h"
#include "ColladaMeshLoader.h"

#include <set>

namespace sge
{

class FullScreenRenderTarget
{
public :
    GLuint textureId = 0;
    GLuint zBufferRbo = 0;
    GLuint fbo = 0;
    
    void create(int width, int height);
    void destroy();
    
    void renderScreenQuad();
};

class GameController : public IGameController
{
    int currentWidth = 0, currentHeight = 0;
    ftype currentTime = 0.0;
    
    mat4 projectionMatrix, viewMatrix, modelMatrix;
    
    std::set<SDL_Keycode> pressedKeys;
    
    bool firstPersonMode = false;
    bool wireframeMode = false;
    bool fogEnabled = false;
    bool physicsDebugMode = true;
    bool enableSimpleBlur = false;
    
    CharacterController player;
    vec3 cameraVector;
    
    std::vector<vec3> cubePositions;
    
    //SimplePhysicsEngine physicsEngine;
    
    SimpleWorldContainer worldContainer;
    
    ScpMeshCollection meshCollection;
    GLSimpleMesh loadedMesh;
    
    std::set<std::string> shaderDefines;
    
    sge::Mesh newMesh;
    
    GLuint vertexShader = 0, fragmentShader = 0;
    GLuint shaderProgram = 0;
    
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

#endif // SGE_GAME_CONTROLLER_H
