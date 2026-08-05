#include "core/window.h"

namespace golias {

    void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        Window* win  = static_cast<Window*>(glfwGetWindowUserPointer(window));
        win->mWidth  = width;
        win->mHeight = height;

        if (win->OnResize) {
            win->OnResize(width, height);
        }
        
    }

    Window::Window(int width, int height, const String title) : mWidth(width), mHeight(height) {

        mWindow = glfwCreateWindow(mWidth, mHeight, title.c_str(), nullptr, nullptr);
        if (!mWindow) {
            LOG_FATAL("Failed to create window.");
        }

        glfwSetWindowUserPointer(mWindow, this);
        glfwSetFramebufferSizeCallback(mWindow, Window::framebuffer_size_callback);

        LOG_INFO("Created window with size {}x{}", mWidth, mHeight);
    }

    Window::~Window() {
        if (mWindow) {
            glfwDestroyWindow(mWindow);
            mWindow = nullptr;
        }

        glfwTerminate();
    }

    void Window::PollEvents() {
        glfwPollEvents();
    }

    bool Window::ShouldClose() const {
        return mWindow && glfwWindowShouldClose(mWindow);
    }

    GLFWwindow* Window::GetNativeWindow() const {
        return mWindow;
    }


        int Window::GetWidth() const {
            return mWidth;
        }
        int Window::GetHeight() const {
            return mHeight;
        }


} // namespace golias
