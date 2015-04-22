#ifndef SHADER_UTILS_H
#define SHADER_UTILS_H

#include "Common.h"

#include <string>

std::string getFileContents(const char* fileName);

std::string getShaderOrProgramLog(GLuint shaderOrProgramObject);

GLuint loadGlShader(const char* fileName, GLenum glShaderType, const char* shaderCodePrefix);

#endif // SHADER_UTILS_H
