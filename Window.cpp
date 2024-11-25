//
// Created by cookieso on 14.08.24.
//
#include "Window.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

void glfwErrorCallback(int error, const char *desc) {
    std::cerr << "Glfw error: " << desc << std::endl;
}

Window::Window() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) throw std::runtime_error("Cannot initialize glfw!");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

    m_Window = glfwCreateWindow(1280, 720, "OpenBFSTM", nullptr, nullptr);
    if (!m_Window) throw std::runtime_error("Cannot create glfw window!");

    glfwMakeContextCurrent(m_Window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
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

void Window::draw() {
    bool someBool;
    {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &someBool);
        ImGui::Checkbox("Another Window", &someBool);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float *) &m_ClearColor);

        if (ImGui::Button("Button"))
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / m_IO->Framerate, m_IO->Framerate);
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
