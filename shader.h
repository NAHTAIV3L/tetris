#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>
#include <stdlib.h>

GLuint CreateShader(const char *vertexPath, const char *fragmentPath, const char* file, size_t line);
void UseShader(GLuint id);

#endif // SHADER_H_
