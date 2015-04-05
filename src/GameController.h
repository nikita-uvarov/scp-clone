#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "SDL.h"
#include "SDL_opengl.h"

#include "IGameController.h"

#include "GLUtils.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <set>

// TODO:
// 1. world simulation: the problem arises when FPS is lower than required number of simulations per second.
//    solution: fixed timestep simulations (possibly >1 or none between frames), capped at 60 FPS
//    basically there is no point in running the game at higher framerates, but we still keep vsync
//    turned off to watch out for performance dynamics.
// 2. 

// Planned graphical engine features:
// 1. Depth fog
// 2. Simple blur
// 3. Flashbang effects

enum Textures
{
    UNIVERSAL,
    MAX_TEXTURES
};

class GameController : public IGameController
{
    int currentWidth, currentHeight;
    double currentTime;
    
    glm::mat4 projectionMatrix, viewMatrix, modelMatrix;
    
    GLTexture texture;
    GLSimpleMesh cube;
    
    std::set<SDL_Keycode> pressedKeys;
    
    bool firstPersonMode;
    bool wireframeMode;
    bool fogEnabled;
    bool physicsDebugMode;
    
    //glm::vec3 playerPosition;
    PhysicalBody player;
    glm::vec3 cameraVector;
    
    std::vector<glm::vec3> cubePositions;
    
    SimplePhysicsEngine physicsEngine;
    
    void updatePlayerDirection();
public :
    void initializeGraphics(int width, int height);
    
    void changeWindowSize(int newWidth, int newHeight);
    
    void renderFrame();
    
    void simulateWorld(double msPassed);
    
    void keyPressed(SDL_Keycode keycode);
    void keyReleased(SDL_Keycode keycode);
    void relativeMouseMotion(int dx, int dy);
};

#endif // GAME_CONTROLLER_H
