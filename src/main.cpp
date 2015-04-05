#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

#include "MainWindow.h"
#include "GameController.h"

int main(int /*argc*/, char** /*argv*/)
{
	/*int done;
	SDL_Window *window;
	SDL_Surface *surface;
	GLuint texture;
	GLfloat texcoords[4];

	surface = SDL_LoadBMP("resources/icon.bmp");
	SDL_VerifyResult(surface, "Unable to load resources/icon.bmp: %s\n", SDL_GetError());
    
	texture = SDL_GL_LoadTexture(surface, texcoords);
	SDL_FreeSurface(surface);*/

	//InitGL(640, 480);
	/*done = 0;

	int framesRendered = 0;
	int t0 = SDL_GetTicks();
    
    bool fullscreen = false;*/
    
    GameController controller;

    MainWindow mainWindow("OpenGL test", &controller);
    
    mainWindow.initializeSDL();
    mainWindow.createWindow();
    mainWindow.startMainLoop();
	
	SDL_Quit();
    
	return 0;
}
