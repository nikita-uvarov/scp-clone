#include "ShaderUtils.h"

#include <cstdio>
#include <memory>
#include <vector>
#include <cerrno>

using namespace std;

string getFileContents(const char* fileName)
{
    FILE* file = fopen(fileName, "r");
    verify(file, "Failed to fopen '%s' for reading: %s", fileName, strerror(errno));
    
    fseek(file, 0, SEEK_END);
    long fileLength = (long)ftell(file);
    fseek(file, 0, SEEK_SET);
    
    vector<char> fileContents(fileLength);
    fread(fileContents.data(), 1, fileLength, file);
    
    return string(fileContents.begin(), fileContents.end());
}

string getShaderOrProgramLog(GLuint object)
{
    GLint logLength = 0;
    
    if (glIsShader(object))
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logLength);
    else if (glIsProgram(object))
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logLength);
    else
        critical_error("%s: An object is not a shader nor a program.", __FUNCTION__);

    vector<char> logContents(logLength);

    if (glIsShader(object))
        glGetShaderInfoLog(object, logLength, nullptr, logContents.data());
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, logLength, nullptr, logContents.data());

    return string(logContents.begin(), logContents.end());
}

GLuint loadGlShader(const char* fileName, GLenum glShaderType, const char* shaderCodePrefix)
{
    string contents = getFileContents(fileName);
    
    GLuint shader = glCreateShader(glShaderType);
    
    const char* sections[] = { shaderCodePrefix, contents.c_str() };
    const GLint lengths[] = { (GLint)strlen(shaderCodePrefix), (GLint)contents.length() };
    
    glShaderSource(shader, 2, sections, lengths);

    glCompileShader(shader);
    
    GLint successful = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &successful);
    
    if (successful == GL_FALSE)
    {
        fprintf(stderr, "%s:", fileName);
        string log = getShaderOrProgramLog(shader);
        glDeleteShader(shader);
        
        critical_error("Failed to load shader '%s': %s", fileName, log.c_str());
    }
    
    return shader;
}
