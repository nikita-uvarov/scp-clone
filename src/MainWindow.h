#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "SDL.h"
#include "SDL_opengl.h"

#include "IGameController.h"

#include <string>

class MainWindow
{
public :
    SDL_Window* sdlWindow;
    IGameController* gameController;
    std::string legacyTitle;
    bool isMainLoopRunning;
    
    bool isFullscreen;
    
    MainWindow(std::string title, IGameController* gameController):
        sdlWindow(nullptr),
        gameController(gameController),
        legacyTitle(title),
        isMainLoopRunning(false),
        isFullscreen(false)
    {}
    
    void initializeSDL();
    
    void createWindow();
    void startMainLoop();
    
    void processEvents();
};

#endif // MAIN_WINDOW_H