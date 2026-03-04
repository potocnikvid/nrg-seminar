#pragma once
#include <glad/glad.h>
#include <string>

GLuint loadHDR(const std::string& path);
GLuint equirectToCubemap(GLuint equirectTex, int size);
