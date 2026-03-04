#pragma once
#include <glad/glad.h>

GLuint generatePrefilteredMap(GLuint envCubemap, int baseSize, int maxMipLevels,
                              int sampleCount = 1024);
