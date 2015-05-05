#ifndef SGE_MAIN_WINDOW_H
#define SGE_MAIN_WINDOW_H

#include "Common.h"

#include "IGameController.h"

#include <string>

namespace sge
{

class MainWindow
{
public :
    SDL_Window* sdlWindow;
    IGameController* gameController;
    std::string legacyTitle;
    bool isMainLoopRunning = false;
    
    bool isFullscreen = false;
    
    bool enablePerformanceProfiling = false;
    
    MainWindow(std::string title, IGameController* gameController):
        sdlWindow(nullptr),
        gameController(gameController),
        legacyTitle(title)
    {}
    
    void initializeSDL();
    
    void createWindow();
    void startMainLoop();
    
    void processEvents();
};

}

#endif // SGE_MAIN_WINDOW_H