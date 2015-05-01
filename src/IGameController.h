#ifndef SGE_I_GAME_CONTROLLER_H
#define SGE_I_GAME_CONTROLLER_H

#include "SDL/SDL.h"

namespace sge
{

class IGameController
{
public :
    virtual ~IGameController() {}
    
    virtual void initializeGraphics(int width, int height) = 0;
    virtual void changeWindowSize(int newWidth, int newHeight) = 0;
    
    virtual void keyPressed(SDL_Keycode keycode) = 0;
    virtual void keyReleased(SDL_Keycode keycode) = 0;
    virtual void relativeMouseMotion(int dx, int dy) = 0;
    
    virtual void simulateWorld(ftype msPassed) = 0;
    virtual void renderFrame() = 0;
};

}

#endif // SGE_I_GAME_CONTROLLER_H