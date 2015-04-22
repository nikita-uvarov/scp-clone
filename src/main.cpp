#include "MainWindow.h"
#include "GameController.h"

using namespace sge;

int main(int /*argc*/, char** /*argv*/)
{   
    GameController controller;

    MainWindow mainWindow("OpenGL test", &controller);
    
    mainWindow.initializeSDL();
    mainWindow.createWindow();
    mainWindow.startMainLoop();
	
	SDL_Quit();
    
	return 0;
}
