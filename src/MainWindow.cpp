#include "MainWindow.h"
#include "PerformanceManager.h"

#include <string>

using namespace std;
using namespace sge;

void MainWindow::initializeSDL()
{
	SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    verify(SDL_Init(SDL_INIT_VIDEO) == 0, "Unable to initialize SDL: %s\n", SDL_GetError());
}

void MainWindow::createWindow()
{
    sdlWindow = SDL_CreateWindow(legacyTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED |  SDL_WINDOW_INPUT_GRABBED);
	verify(sdlWindow, "Unable to create OpenGL sdlWindow: %s\n", SDL_GetError());
    
    verify(SDL_GL_CreateContext(sdlWindow), "Unable to create OpenGL context: %s\n", SDL_GetError());
    
	SDL_GL_SetSwapInterval(0);
    
    gameController->initializeGraphics(640, 480);
    
    SDL_SetRelativeMouseMode(SDL_TRUE);
    //SDL_ShowCursor(0);
    //SDL_WM_GrabInput( SDL_GRAB_ON );
}

void MainWindow::startMainLoop()
{
    isMainLoopRunning = true;
    
    int framesRendered = 0;
    int lastFpsUpdate = SDL_GetTicks();
    ftype lastLogicUpdate = lastFpsUpdate;
    
    ftype elementaryLogicTicks = 1000.0 / 60.0;
    
    //elementaryLogicTicks = 1000.0 / 60.0;
    
	while (isMainLoopRunning)
	{
		framesRendered++;
		int currentTicks = SDL_GetTicks();

		while (currentTicks - lastFpsUpdate > 1000.0)
		{
            if (enablePerformanceProfiling)
                PerformanceManager::instance().updatePerformanceInformation();
            
			ftype fps = framesRendered * 1000.0 / (currentTicks - lastFpsUpdate);
            
            char fpsString[256];
            sprintf(fpsString, "%.1lf", fps);
            
            ftype msPerFrame = (currentTicks - lastFpsUpdate) / (ftype)framesRendered;
            
            char mspfString[256];
            sprintf(mspfString, "%.2lf", msPerFrame);
            
			std::string performanceString =
                "FPS: " + string(fpsString) + ", " +
                string(mspfString) + " ms/frame";
                
            if (enablePerformanceProfiling)
            {
                performanceString += ", " + PerformanceManager::instance().formatPerformanceString();
            }
            else
            {
                performanceString += ", [press i to enable perf profiling]";
            }
            
			SDL_SetWindowTitle(sdlWindow, (legacyTitle + " [" + performanceString + "]").c_str());
			lastFpsUpdate = currentTicks;
			framesRendered = 0;
            
            // also flush streams occasionally to see debug output
            fflush(stdout);
            fflush(stderr);
		}
		
		int nTicksMax = 1;
        int nTicksSimulated = 0;
		
		while ((currentTicks - lastLogicUpdate) > elementaryLogicTicks)
        {
            gameController->simulateWorld(elementaryLogicTicks);
            gameController->simulateWorld(elementaryLogicTicks);
            //gameController->simulateWorld(elementaryLogicTicks);
            //gameController->simulateWorld(elementaryLogicTicks);
            lastLogicUpdate += elementaryLogicTicks;
            
            nTicksSimulated++;
            if (nTicksSimulated >= nTicksMax) break;
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
                
                // change size event will be dispatched automatically
                
                if (isFullscreen)
                    SDL_SetWindowFullscreen(sdlWindow, SDL_WINDOW_FULLSCREEN);
                else
                    SDL_SetWindowFullscreen(sdlWindow, 0);
            }
            
            if (event.key.keysym.sym == SDLK_i)
            {
                enablePerformanceProfiling = !enablePerformanceProfiling;
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
