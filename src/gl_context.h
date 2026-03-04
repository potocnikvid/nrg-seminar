#pragma once
#include <glad/glad.h>
#include <string>

GLuint loadShader(const std::string& vertPath, const std::string& fragPath);
void   renderCube();
void   renderQuad();
