//
// Created by cookieso on 14.08.24.
//

#ifndef OPENBFSTM_WINDOW_H
#define OPENBFSTM_WINDOW_H

#include <iostream>
#include <imgui.h>

class GLFWwindow;

class Window {
public:
    Window();

    ~Window();

    bool shouldClose();

    void preDraw();

    void draw();

    void afterDraw();
private:
    GLFWwindow *m_Window;
    ImGuiIO *m_IO;
    ImVec4 m_ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

#endif //OPENBFSTM_WINDOW_H
