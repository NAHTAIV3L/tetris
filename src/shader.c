#include "shader.h"

#include <stdio.h>

char *ReadFile(const char *file)
{

    char *buffer = 0;
    long length = 0;
    FILE *f = fopen(file, "r");

    if (f)
    {
        while (fgetc(f) != EOF)
        {
            length++;
        }
        fclose(f);
    }

    f = fopen(file, "r");
    if (f)
    {
        buffer = malloc(length + 1);
        if (buffer)
        {
            char c = 0;
            for (int i = 0; i < length; i++)
            {
                c = fgetc(f);
                if (c == EOF)
                {
                    break;
                }
                buffer[i] = c;
            }
            buffer[length] = 0x00;
        }
        fclose(f);
    }
    return buffer;
}

GLuint CreateShader(const char *vertexPath, const char *fragmentPath, const char* file, size_t line)
{
    const char *vertexCode = ReadFile(vertexPath);
    const char *fragmentCode = ReadFile(fragmentPath);

    GLuint vertex, fragment;
    int success;
    char infoLog[512];

    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexCode, NULL);
    glCompileShader(vertex);

    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("ERROR: Failed to Compile %s shader file%s\n", vertexPath, infoLog);
    }

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentCode, NULL);
    glCompileShader(fragment);

    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        printf("ERROR: Failed to Compile %s shader file%s\n", vertexPath, infoLog);
    }

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, vertex);
    glAttachShader(ShaderProgram, fragment);
    glLinkProgram(ShaderProgram);

    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        printf("%s:%ld: Shader Link Failed\n", file, line);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    free((void *)vertexCode);
    free((void *)fragmentCode);

    return ShaderProgram;
}

void UseShader(GLuint ID)
{
    glUseProgram(ID);
}
