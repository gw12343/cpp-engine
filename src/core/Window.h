#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>
#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    bool Initialize();
    void Update();
    bool ShouldClose() const;
    void SwapBuffers() const;
    void PollEvents() const;
    void Shutdown();
    void OnResize(int width, int height);

    int GetWidth() const;
    int GetHeight() const;

    GLFWwindow* GetNativeWindow() const { return m_window; }

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    std::string m_title;
    bool m_initialized;

    bool InitGLFW();
    bool InitGLAD();
    bool InitImGui();
};
