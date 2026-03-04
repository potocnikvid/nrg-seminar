#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "gl_context.h"
#include "env_map.h"
#include "irradiance.h"
#include "prefilter.h"
#include "brdf_lut.h"
#include "pbr_renderer.h"
#include "ui.h"

namespace {
    Camera camera;
    bool   mousePressed = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    int    windowW = 1280, windowH = 720;
}

static void framebufferSizeCallback(GLFWwindow*, int w, int h) {
    windowW = w; windowH = h;
    glViewport(0, 0, w, h);
}

static void scrollCallback(GLFWwindow*, double, double yoff) {
    camera.distance -= static_cast<float>(yoff) * 0.5f;
    if (camera.distance < 1.0f) camera.distance = 1.0f;
    if (camera.distance > 50.0f) camera.distance = 50.0f;
}

static void mouseButtonCallback(GLFWwindow* w, int button, int action, int) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mousePressed = true;
            glfwGetCursorPos(w, &lastMouseX, &lastMouseY);
        } else {
            mousePressed = false;
        }
    }
}

static void cursorPosCallback(GLFWwindow*, double xpos, double ypos) {
    if (!mousePressed) return;
    float dx = static_cast<float>(xpos - lastMouseX);
    float dy = static_cast<float>(ypos - lastMouseY);
    camera.yaw   += dx * 0.3f;
    camera.pitch  += dy * 0.3f;
    camera.pitch  = glm::clamp(camera.pitch, -89.0f, 89.0f);
    lastMouseX = xpos;
    lastMouseY = ypos;
}

static std::vector<std::string> findHDRFiles(const std::string& dir) {
    std::vector<std::string> result;
    if (!std::filesystem::exists(dir)) return result;
    for (auto& entry : std::filesystem::directory_iterator(dir)) {
        auto ext = entry.path().extension().string();
        if (ext == ".hdr" || ext == ".HDR")
            result.push_back(entry.path().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}

struct IBLMaps {
    GLuint envCubemap;
    GLuint irradianceMap;
    GLuint prefilteredMap;
};

static const int cubemapSizeOptions[]    = { 128, 256, 512, 1024 };
static const int irradianceSizeOptions[] = { 16, 32, 64 };
static const int prefilterSizeOptions[]  = { 64, 128, 256 };
static const int sampleCountOptions[]    = { 256, 512, 1024, 2048 };
static const int lutSizeOptions[]        = { 128, 256, 512 };

static IBLMaps generateIBLMaps(GLuint hdrTex, const UIState& ui) {
    IBLMaps m{};
    m.envCubemap     = equirectToCubemap(hdrTex, cubemapSizeOptions[ui.cubemapSizeIdx]);
    m.irradianceMap  = generateIrradianceMap(m.envCubemap, irradianceSizeOptions[ui.irradianceSizeIdx]);
    m.prefilteredMap = generatePrefilteredMap(m.envCubemap,
                           prefilterSizeOptions[ui.prefilterSizeIdx], 5,
                           sampleCountOptions[ui.prefilterSamplesIdx]);
    return m;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(windowW, windowH, "IBL PBR Renderer", nullptr, nullptr);
    if (!window) { std::cerr << "Window creation failed\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n"; return 1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    initUI(window);

    auto hdrFiles = findHDRFiles("assets/env");
    if (hdrFiles.empty()) {
        std::cerr << "No .hdr files found in assets/env/\n"
                  << "Download an HDRI from https://polyhaven.com/hdris and place it there.\n";
        shutdownUI();
        glfwTerminate();
        return 1;
    }

    std::vector<const char*> envNames;
    std::vector<std::string> envNameStrings;
    for (auto& p : hdrFiles) {
        envNameStrings.push_back(std::filesystem::path(p).stem().string());
    }
    for (auto& s : envNameStrings) envNames.push_back(s.c_str());

    UIState ui;

    GLuint hdrTex = loadHDR(hdrFiles[0]);
    IBLMaps ibl = generateIBLMaps(hdrTex, ui);
    GLuint brdfLUT = generateBRDFLUT(lutSizeOptions[ui.lutSizeIdx]);

    int prevEnvIndex = 0;

    Mesh meshes[3];
    meshes[0] = createSphere(64, 64);
    meshes[1] = createCube();
    meshes[2] = createTorus();
    GLuint pbrShader = loadShader("shaders/pbr.vert", "shaders/pbr.frag");
    GLuint skyboxShader = loadShader("shaders/skybox.vert", "shaders/skybox.frag");

    glViewport(0, 0, windowW, windowH);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (ui.envIndex != prevEnvIndex) {
            glDeleteTextures(1, &hdrTex);
            glDeleteTextures(1, &ibl.envCubemap);
            glDeleteTextures(1, &ibl.irradianceMap);
            glDeleteTextures(1, &ibl.prefilteredMap);
            hdrTex = loadHDR(hdrFiles[ui.envIndex]);
            ibl = generateIBLMaps(hdrTex, ui);
            prevEnvIndex = ui.envIndex;
        }

        if (ui.regenerateIBL) {
            ui.regenerateIBL = false;
            glDeleteTextures(1, &ibl.envCubemap);
            glDeleteTextures(1, &ibl.irradianceMap);
            glDeleteTextures(1, &ibl.prefilteredMap);
            glDeleteTextures(1, &brdfLUT);
            ibl = generateIBLMaps(hdrTex, ui);
            brdfLUT = generateBRDFLUT(lutSizeOptions[ui.lutSizeIdx]);
            glViewport(0, 0, windowW, windowH);
        }

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(
            static_cast<float>(windowW) / static_cast<float>(windowH));

        glUseProgram(pbrShader);
        glUniformMatrix4fv(glGetUniformLocation(pbrShader, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(pbrShader, "projection"), 1, GL_FALSE, &proj[0][0]);
        glm::vec3 camPos = camera.getPosition();
        glUniform3fv(glGetUniformLocation(pbrShader, "camPos"), 1, &camPos[0]);
        glUniform1f(glGetUniformLocation(pbrShader, "exposure"), ui.exposure);
        glUniform1i(glGetUniformLocation(pbrShader, "showDiffuse"),  ui.showDiffuse  ? 1 : 0);
        glUniform1i(glGetUniformLocation(pbrShader, "showSpecular"), ui.showSpecular ? 1 : 0);
        glUniform1i(glGetUniformLocation(pbrShader, "tonemapMode"), ui.tonemapMode);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.irradianceMap);
        glUniform1i(glGetUniformLocation(pbrShader, "irradianceMap"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.prefilteredMap);
        glUniform1i(glGetUniformLocation(pbrShader, "prefilterMap"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, brdfLUT);
        glUniform1i(glGetUniformLocation(pbrShader, "brdfLUT"), 2);

        const Mesh& activeMesh = meshes[ui.sceneObject];

        if (ui.gridMode) {
            int nRows = 7, nCols = 7;
            float spacing = 2.5f;
            for (int row = 0; row < nRows; ++row) {
                float metallic = static_cast<float>(row) / static_cast<float>(nRows - 1);
                for (int col = 0; col < nCols; ++col) {
                    float roughness = glm::clamp(
                        static_cast<float>(col) / static_cast<float>(nCols - 1), 0.05f, 1.0f);
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(
                        (col - nCols / 2) * spacing,
                        (row - nRows / 2) * spacing,
                        0.0f));
                    glUniformMatrix4fv(glGetUniformLocation(pbrShader, "model"),
                                       1, GL_FALSE, &model[0][0]);
                    glUniform3fv(glGetUniformLocation(pbrShader, "albedo"), 1, &ui.albedo[0]);
                    glUniform1f(glGetUniformLocation(pbrShader, "metallic"), metallic);
                    glUniform1f(glGetUniformLocation(pbrShader, "roughness"), roughness);

                    glBindVertexArray(activeMesh.vao);
                    glDrawElements(GL_TRIANGLES, activeMesh.indexCount, GL_UNSIGNED_INT, nullptr);
                }
            }
        } else {
            glm::mat4 model = glm::mat4(1.0f);
            glUniformMatrix4fv(glGetUniformLocation(pbrShader, "model"),
                               1, GL_FALSE, &model[0][0]);
            glUniform3fv(glGetUniformLocation(pbrShader, "albedo"), 1, &ui.albedo[0]);
            glUniform1f(glGetUniformLocation(pbrShader, "metallic"), ui.metallic);
            glUniform1f(glGetUniformLocation(pbrShader, "roughness"),
                        glm::clamp(ui.roughness, 0.05f, 1.0f));

            glBindVertexArray(activeMesh.vao);
            glDrawElements(GL_TRIANGLES, activeMesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }

        if (ui.showBackground) {
            glUseProgram(skyboxShader);
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"),
                               1, GL_FALSE, &view[0][0]);
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"),
                               1, GL_FALSE, &proj[0][0]);
            glUniform1f(glGetUniformLocation(skyboxShader, "exposure"), ui.exposure);
            glUniform1i(glGetUniformLocation(skyboxShader, "tonemapMode"), ui.tonemapMode);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envCubemap);
            glUniform1i(glGetUniformLocation(skyboxShader, "environmentMap"), 0);
            renderCube();
        }

        drawUI(ui, envNames.data(), static_cast<int>(envNames.size()));

        glfwSwapBuffers(window);
    }

    shutdownUI();
    glfwTerminate();
    return 0;
}
