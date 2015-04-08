#include "ShaderUtils.h"

#include <stdio.h>

char* getFileContents(const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    
    // FIXME: error handling
    SDL_assert(file);
    
    fseek(file, 0, SEEK_END);
    int fileLength = (int)ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* fileContents = new char[fileLength + 1];
    fread(fileContents, 1, fileLength, file);
    fileContents[fileLength] = 0;
    
    return fileContents;
}

char* getShaderLog(GLuint object)
{
    GLint logLength = 0;
    
    if (glIsShader(object))
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logLength);
    else if (glIsProgram(object))
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
    else
        SDL_assert(!"not a shader or program");

    char* log = new char[logLength];

    if (glIsShader(object))
        glGetShaderInfoLog(object, logLength, nullptr, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, logLength, nullptr, log);

    return log;
}

GLuint loadGlShader(const char* fileName, GLenum glShaderType, const char* shaderCodePrefix)
{
    const char* contents = getFileContents(fileName);
    
    GLuint shader = glCreateShader(glShaderType);
    int length = (int)strlen(contents);
    
    const char* sections[] = { shaderCodePrefix, contents };
    const int lengths[] = { strlen(shaderCodePrefix), length };
    
    glShaderSource(shader, 2, sections, lengths);
    delete[] contents;

    glCompileShader(shader);
    GLint successful = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &successful);
    if (successful == GL_FALSE)
    {
        fprintf(stderr, "%s:", fileName);
        char* log = getShaderLog(shader);
        fprintf(stderr, "%s\n", log);
        delete[] log;
        glDeleteShader(shader);
        
        // FIXME: error handling
        SDL_assert(!"shader not loaded");
    }
    
    return shader;
}
