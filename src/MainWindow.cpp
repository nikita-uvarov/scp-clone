#include "MainWindow.h"
#include "SDLUtils.h"

#include <string>

using namespace std;

void MainWindow::initializeSDL()
{
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    SDL_VerifyResult(SDL_Init(SDL_INIT_VIDEO) == 0, "Unable to initialize SDL: %s\n", SDL_GetError());
}

void MainWindow::createWindow()
{
    sdlWindow = SDL_CreateWindow(legacyTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	SDL_VerifyResult(sdlWindow, "Unable to create OpenGL sdlWindow: %s\n", SDL_GetError());
    
    SDL_VerifyResult(SDL_GL_CreateContext(sdlWindow), "Unable to create OpenGL context: %s\n", SDL_GetError());
    
	SDL_GL_SetSwapInterval(0);
    
    gameController->initializeGraphics(640, 480);
    
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void MainWindow::startMainLoop()
{
    isMainLoopRunning = true;
    
    int framesRendered = 0;
    int lastFpsUpdate = SDL_GetTicks();
    double lastLogicUpdate = lastFpsUpdate;
    
    double elementaryLogicTicks = 1000.0 / 60.0;
    
	while (isMainLoopRunning)
	{
		framesRendered++;
		int currentTicks = SDL_GetTicks();

		while (currentTicks - lastFpsUpdate > 1000.0)
		{
			double fps = framesRendered * 1000.0 / (currentTicks - lastFpsUpdate);
			std::string fpsString = "FPS: " + std::to_string(fps);
			SDL_SetWindowTitle(sdlWindow, (legacyTitle + " [" + fpsString + "]").c_str());
			lastFpsUpdate = currentTicks;
			framesRendered = 0;
		}
		
		while ((currentTicks - lastLogicUpdate) > elementaryLogicTicks)
        {
            gameController->simulateWorld(elementaryLogicTicks);
            lastLogicUpdate += elementaryLogicTicks;
        }
		
		processEvents();

		gameController->renderFrame();
        SDL_GL_SwapWindow(sdlWindow);
	}
}

void MainWindow::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            isMainLoopRunning = false;
        }
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_ESCAPE)
            {
                isMainLoopRunning = false;
            }
            
            if (event.key.keysym.sym == SDLK_F1)
            {
                isFullscreen = !isFullscreen;
                
                if (isFullscreen)
                    SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN);
                else
                    SDL_SetWindowFullscreen(sdlWindow, 0);
            }
            
            gameController->keyPressed(event.key.keysym.sym);
        }
        if (event.type == SDL_KEYUP)
        {
            gameController->keyReleased(event.key.keysym.sym);
        }
        
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
            gameController->changeWindowSize(event.window.data1, event.window.data2);
        }
        
        if (event.type == SDL_MOUSEMOTION)
        {
            int dx = event.motion.xrel;
            int dy = event.motion.yrel; 
            
            gameController->relativeMouseMotion(dx, dy);
        }
    }
}
