//
// Created by cookieso on 14.08.24.
//
#include "Window.h"
#include "playback/AudioPlayback.h"
#include "implot/implot.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

void glfwErrorCallback(int error, const char *desc) {
    std::cerr << "Glfw error: " << desc << std::endl;
}

void dropCallback(GLFWwindow *window, int count, const char *paths[]) {

}

Window::Window() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) throw std::runtime_error("Cannot initialize glfw!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    m_Window = glfwCreateWindow(1280, 720, "OpenBFSTM", nullptr, nullptr);
    if (!m_Window) throw std::runtime_error("Cannot create glfw window!");

    glfwMakeContextCurrent(m_Window);
    glfwSetDropCallback(m_Window, dropCallback);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    m_IO = &ImGui::GetIO();

    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.AntiAliasedLinesUseTex = true;

    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
}

Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Window::shouldClose() {
    return glfwWindowShouldClose(m_Window);
}

void Window::preDraw() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Window::draw(AudioPlayback& audio, BfstmContext& context) {
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");

        uint32_t sampleCount = (context.streamInfo.blockSizeSamples * (context.streamInfo.blockCountPerChannel - 1) +
                           context.streamInfo.lastBlockSizeSamples);
        ImGui::Text("Length %is",  sampleCount / context.streamInfo.sampleRate);
        ImGui::Text("Loop start sample %i", context.streamInfo.loopStart);
        ImGui::Text("Loop end sample %i", context.streamInfo.loopEnd);

        if (ImPlot::BeginPlot("##SampleDiagram")) {
            //ImPlot::PlotLine("##Samples", nullptr, nullptr, sampleCount);
            ImPlot::EndPlot();
        }

        if (ImGui::Button("Pause")) {
            audio.pause(!audio.isPaused());
        }
        if (context.streamInfo.channelNum > 2) {
            ImGui::Text("Channels Groups: ");
            for (int i = 0; i < context.streamInfo.channelNum; i += 2) {
                ImGui::SameLine();
                if (ImGui::Button(std::format("Channel Group {}", i/2).c_str())) {
                    audio.setChannel(i);
                }
            }
        }
        if (!context.regionInfos.empty()) {
            if (ImGui::Button("Increment region")) {
                audio.incRegion();
            }
        }
        ImGui::End();
    }
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_Window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(m_ClearColor.x * m_ClearColor.w, m_ClearColor.y * m_ClearColor.w, m_ClearColor.z * m_ClearColor.w,
                 m_ClearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Window::afterDraw() {
    glfwSwapBuffers(m_Window);
}
