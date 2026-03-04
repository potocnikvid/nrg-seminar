#include "ui.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

void initUI(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::StyleColorsDark();
}

void drawUI(UIState& state, const char** envNames, int envCount) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("IBL Controls");

    ImGui::ColorEdit3("Albedo", &state.albedo.x);
    ImGui::SliderFloat("Roughness", &state.roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Metallic",  &state.metallic,  0.0f, 1.0f);
    ImGui::SliderFloat("Exposure",  &state.exposure,  0.1f, 5.0f);

    const char* tonemapNames[] = { "Reinhard", "ACES", "Uncharted2" };
    ImGui::Combo("Tonemap", &state.tonemapMode, tonemapNames, 3);

    const char* objectNames[] = { "Sphere", "Cube", "Torus" };
    ImGui::Combo("Object", &state.sceneObject, objectNames, 3);

    ImGui::Separator();
    ImGui::Checkbox("Diffuse IBL",   &state.showDiffuse);
    ImGui::Checkbox("Specular IBL",  &state.showSpecular);
    ImGui::Checkbox("Background",    &state.showBackground);
    ImGui::Checkbox("Grid Mode",     &state.gridMode);
    ImGui::Separator();
    if (envCount > 0)
        ImGui::Combo("Environment", &state.envIndex, envNames, envCount);

    if (ImGui::CollapsingHeader("IBL Parameters")) {
        const char* cubemapSizes[]    = { "128", "256", "512", "1024" };
        const char* irradianceSizes[] = { "16", "32", "64" };
        const char* prefilterSizes[]  = { "64", "128", "256" };
        const char* sampleCounts[]    = { "256", "512", "1024", "2048" };
        const char* lutSizes[]        = { "128", "256", "512" };

        bool changed = false;
        changed |= ImGui::Combo("Cubemap Size",    &state.cubemapSizeIdx,    cubemapSizes, 4);
        changed |= ImGui::Combo("Irradiance Size", &state.irradianceSizeIdx, irradianceSizes, 3);
        changed |= ImGui::Combo("Prefilter Size",  &state.prefilterSizeIdx,  prefilterSizes, 3);
        changed |= ImGui::Combo("Prefilter Samples", &state.prefilterSamplesIdx, sampleCounts, 4);
        changed |= ImGui::Combo("LUT Size",        &state.lutSizeIdx,        lutSizes, 3);

        if (changed)
            state.regenerateIBL = true;
    }

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void shutdownUI() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
