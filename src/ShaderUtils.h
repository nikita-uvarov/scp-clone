#ifndef SGE_SHADER_UTILS_H
#define SGE_SHADER_UTILS_H

#include "Common.h"

#include <string>

std::string getFileContents(const char* fileName);

std::string getShaderOrProgramLog(GLuint shaderOrProgramObject);

GLuint loadGlShader(const char* fileName, GLenum glShaderType, const char* shaderCodePrefix);

#endif // SGE_SHADER_UTILS_H
