#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "SDL.h"
#include "SDL_opengl.h"

#include "IGameController.h"
#include "GLUtils.h"
#include "ScpMeshCollection.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <set>

// TODO:
// 1. world simulation: the problem arises when FPS is lower than required number of simulations per second.
//    solution: fixed timestep simulations (possibly >1 or none between frames), capped at 60 FPS
//    basically there is no point in running the game at higher framerates, but we still keep vsync
//    turned off to watch out for performance dynamics. - kinda done
// 2. world organization: meshes contain both physics & graphics information;
//    the world is optimized to display a lot of similar meshes
//    whole staircase can be stored in memory, because differenet heuristical
//    optimizations are performed to lower physics / graphics processing times

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
    //GLSimpleMesh cube;
    
    std::set<SDL_Keycode> pressedKeys;
    
    bool firstPersonMode;
    bool wireframeMode;
    bool fogEnabled;
    bool physicsDebugMode;
    
    //glm::vec3 playerPosition;
    CharacterController player;
    glm::vec3 cameraVector;
    
    std::vector<glm::vec3> cubePositions;
    
    //SimplePhysicsEngine physicsEngine;
    
    SimpleWorldContainer worldContainer;
    
    SimpleTextureManager textureManager;
    ScpMeshCollection meshCollection;
    
    GLuint fragmentShader;
    
    void updatePlayerDirection();
public :
    GameController();
    
    void initializeGraphics(int width, int height);
    
    void changeWindowSize(int newWidth, int newHeight);
    
    void renderFrame();
    
    void simulateWorld(double msPassed);
    
    void keyPressed(SDL_Keycode keycode);
    void keyReleased(SDL_Keycode keycode);
    void relativeMouseMotion(int dx, int dy);
};

#endif // GAME_CONTROLLER_H
