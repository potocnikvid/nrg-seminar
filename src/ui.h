#pragma once
#include <glm/glm.hpp>

struct GLFWwindow;

struct UIState {
    float     roughness     = 0.5f;
    float     metallic      = 0.0f;
    float     exposure      = 1.0f;
    glm::vec3 albedo        = glm::vec3(0.5f, 0.0f, 0.0f);
    bool      showDiffuse   = true;
    bool      showSpecular  = true;
    bool      showBackground = true;
    bool      gridMode      = true;
    int       envIndex      = 0;
    int       tonemapMode   = 0;
    int       sceneObject   = 0;

    int       cubemapSizeIdx    = 2;
    int       irradianceSizeIdx = 1;
    int       prefilterSizeIdx  = 1;
    int       prefilterSamplesIdx = 2;
    int       lutSizeIdx        = 2;
    bool      regenerateIBL     = false;
};

void initUI(GLFWwindow* window);
void drawUI(UIState& state, const char** envNames, int envCount);
void shutdownUI();
