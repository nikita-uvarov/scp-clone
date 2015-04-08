#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "SDL.h"
#include "SDL_opengl.h"

char* getFileContents(const char* fileName);

char* getShaderLog(GLuint shaderOrProgramObject);

GLuint loadGlShader(const char* fileName, GLenum glShaderType, const char* shaderCodePrefix);

#endif // SHADER_UTILS_H