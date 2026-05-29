#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <algorithm>

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
    using clk = std::chrono::high_resolution_clock;
    auto ms = [](clk::duration d) {
        return std::chrono::duration<double, std::milli>(d).count();
    };

    IBLMaps m{};
    const int cubeSz = cubemapSizeOptions[ui.cubemapSizeIdx];
    const int irrSz  = irradianceSizeOptions[ui.irradianceSizeIdx];
    const int preSz  = prefilterSizeOptions[ui.prefilterSizeIdx];
    const int preSm  = sampleCountOptions[ui.prefilterSamplesIdx];

    auto t0 = clk::now();
    m.envCubemap = equirectToCubemap(hdrTex, cubeSz);
    glFinish();
    auto t1 = clk::now();
    m.irradianceMap = generateIrradianceMap(m.envCubemap, irrSz);
    glFinish();
    auto t2 = clk::now();
    m.prefilteredMap = generatePrefilteredMap(m.envCubemap, preSz, 5, preSm);
    glFinish();
    auto t3 = clk::now();

    std::cout << "[IBL] equirect->cube  " << cubeSz << "^2          : "
              << ms(t1 - t0) << " ms\n";
    std::cout << "[IBL] irradiance      " << irrSz << "^2           : "
              << ms(t2 - t1) << " ms\n";
    std::cout << "[IBL] prefilter       " << preSz << "^2 5mip "
              << preSm << "spp : " << ms(t3 - t2) << " ms\n";
    std::cout << "[IBL] total preprocessing                : "
              << ms(t3 - t0) << " ms" << std::endl;
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

    auto tLut0 = std::chrono::high_resolution_clock::now();
    GLuint brdfLUT = generateBRDFLUT(lutSizeOptions[ui.lutSizeIdx]);
    glFinish();
    auto tLut1 = std::chrono::high_resolution_clock::now();
    std::cout << "[IBL] BRDF LUT       " << lutSizeOptions[ui.lutSizeIdx]
              << "^2 1024spp     : "
              << std::chrono::duration<double, std::milli>(tLut1 - tLut0).count()
              << " ms" << std::endl;

    int prevEnvIndex = 0;

    Mesh meshes[3];
    meshes[0] = createSphere(64, 64);
    meshes[1] = createCube();
    meshes[2] = createTorus();
    GLuint pbrShader = loadShader("shaders/pbr.vert", "shaders/pbr.frag");
    GLuint skyboxShader = loadShader("shaders/skybox.vert", "shaders/skybox.frag");

    glViewport(0, 0, windowW, windowH);

    struct BenchConfig { const char* name; bool grid; bool spec; };
    const BenchConfig benchConfigs[] = {
        {"single, diff+spec", false, true },
        {"single, diff only", false, false},
        {"grid,   diff+spec", true,  true },
        {"grid,   diff only", true,  false},
    };
    const int benchWarmup = 120, benchMeasure = 3000;
    bool benchActive = false;
    int  benchCfg = 0, benchFrame = 0;
    std::vector<double> benchTimes(benchMeasure, 0.0);
    bool benchSavedGrid = false, benchSavedSpec = true;
    std::chrono::high_resolution_clock::time_point benchFrameStart;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (ui.vsyncChanged) {
            ui.vsyncChanged = false;
            glfwSwapInterval(ui.vsync ? 1 : 0);
        }

        if (ui.benchmarkRequest && !benchActive) {
            ui.benchmarkRequest = false;
            benchActive   = true;
            benchCfg      = 0;
            benchFrame    = 0;
            benchSavedGrid = ui.gridMode;
            benchSavedSpec = ui.showSpecular;
            glfwSwapInterval(0);
            std::cout << "\n[bench] starting benchmark (vsync off, "
                      << benchWarmup << " warmup + " << benchMeasure
                      << " measured frames per config)\n";
        }

        if (benchActive) {
            ui.gridMode     = benchConfigs[benchCfg].grid;
            ui.showSpecular = benchConfigs[benchCfg].spec;
            benchFrameStart = std::chrono::high_resolution_clock::now();
        }

        if (ui.envIndex != prevEnvIndex) {
            glDeleteTextures(1, &hdrTex);
            glDeleteTextures(1, &ibl.envCubemap);
            glDeleteTextures(1, &ibl.irradianceMap);
            glDeleteTextures(1, &ibl.prefilteredMap);
            hdrTex = loadHDR(hdrFiles[ui.envIndex]);
            ibl = generateIBLMaps(hdrTex, ui);
            prevEnvIndex = ui.envIndex;
            glViewport(0, 0, windowW, windowH);
        }

        if (ui.regenerateIBL) {
            ui.regenerateIBL = false;
            glDeleteTextures(1, &ibl.envCubemap);
            glDeleteTextures(1, &ibl.irradianceMap);
            glDeleteTextures(1, &ibl.prefilteredMap);
            glDeleteTextures(1, &brdfLUT);
            ibl = generateIBLMaps(hdrTex, ui);
            auto rt0 = std::chrono::high_resolution_clock::now();
            brdfLUT = generateBRDFLUT(lutSizeOptions[ui.lutSizeIdx]);
            glFinish();
            auto rt1 = std::chrono::high_resolution_clock::now();
            std::cout << "[IBL] BRDF LUT       "
                      << lutSizeOptions[ui.lutSizeIdx]
                      << "^2 1024spp     : "
                      << std::chrono::duration<double, std::milli>(rt1 - rt0).count()
                      << " ms" << std::endl;
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

        if (benchActive) {
            glFinish();
            auto t1 = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(t1 - benchFrameStart).count();
            if (benchFrame >= benchWarmup) {
                benchTimes[benchFrame - benchWarmup] = ms;
            }
            benchFrame++;

            if (benchFrame >= benchWarmup + benchMeasure) {
                std::vector<double> sorted = benchTimes;
                std::sort(sorted.begin(), sorted.end());
                double median = sorted[benchMeasure / 2];
                double p1     = sorted[(int)(benchMeasure * 0.01)];
                double p99    = sorted[(int)(benchMeasure * 0.99)];
                double sum = 0;
                for (double t : benchTimes) sum += t;
                double mean = sum / benchMeasure;
                std::printf("[bench] %-20s  median=%6.3f ms  (p1=%5.3f  p99=%6.3f  mean=%6.3f)  -> %5.0f FPS\n",
                            benchConfigs[benchCfg].name, median, p1, p99, mean, 1000.0 / median);
                std::fflush(stdout);
                benchFrame = 0;
                benchCfg++;
                if (benchCfg >= (int)(sizeof(benchConfigs) / sizeof(benchConfigs[0]))) {
                    benchActive    = false;
                    ui.gridMode    = benchSavedGrid;
                    ui.showSpecular = benchSavedSpec;
                    glfwSwapInterval(ui.vsync ? 1 : 0);
                    std::cout << "[bench] done\n" << std::endl;
                }
            }
        }
    }

    shutdownUI();
    glfwTerminate();
    return 0;
}
