#include "core/wsi/glfw_window.h"


#if defined(GOLIAS_PLATFORM_OSX)
    #define GLFW_EXPOSE_NATIVE_COCOA
    #include <GLFW/glfw3native.h>
#endif



namespace golias {

    void GLFW_Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
        GLFW_Window* win  = static_cast<GLFW_Window*>(glfwGetWindowUserPointer(window));
        win->mWidth  = width;
        win->mHeight = height;

        if (win->OnResize) {
            win->OnResize(width, height);
        }
    }

    GLFW_Window::GLFW_Window(int width, int height, const String title) : mWidth(width), mHeight(height), mTitle(title) {
#if defined(GOLIAS_PLATFORM_OSX)
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
#endif
        if (!glfwInit()) {
            LOG_FATAL("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        mWindow = glfwCreateWindow(mWidth, mHeight, title.c_str(), nullptr, nullptr);
        if (!mWindow) {
            LOG_FATAL("Failed to create window.");
        }

        glfwSetWindowUserPointer(mWindow, this);
        glfwSetFramebufferSizeCallback(mWindow, GLFW_Window::framebuffer_size_callback);

        LOG_INFO("Created window with size {}x{}", mWidth, mHeight);
    }

    GLFW_Window::~GLFW_Window() {
        if (mWindow) {
            glfwDestroyWindow(mWindow);
            mWindow = nullptr;
        }

        glfwTerminate();
    }

    void GLFW_Window::PollEvents() {
        glfwPollEvents();
    }

    bool GLFW_Window::ShouldClose() const {
        return mWindow && glfwWindowShouldClose(mWindow);
    }

    void GLFW_Window::SetTitle(const String& title) {
        mTitle = title;
        if (mWindow) {
            glfwSetWindowTitle(mWindow, mTitle.c_str());
        }
    }

    void GLFW_Window::GetFramebufferSize(int* width, int* height) const {
        if (mWindow) {
            glfwGetFramebufferSize(mWindow, width, height);
        } else {
            *width  = 0;
            *height = 0;
        }
    }

    void GLFW_Window::WaitForEvents() {
        glfwWaitEvents();
    }

    void* GLFW_Window::GetNativeHandle() const {
#if defined(GOLIAS_PLATFORM_OSX)
        if (!mWindow || glfwGetPlatform() != GLFW_PLATFORM_COCOA) {
            return nullptr;
        }
        return glfwGetCocoaWindow(mWindow);
#else
        return nullptr;
#endif
    }

    void* GLFW_Window::GetNativeViewHandle() const {
#if defined(GOLIAS_PLATFORM_OSX)
        if (!mWindow || glfwGetPlatform() != GLFW_PLATFORM_COCOA) {
            return nullptr;
        }

        return glfwGetCocoaView(mWindow);
#else
        return nullptr;
#endif
    }

    void* GLFW_Window::GetHandle() const {
        return mWindow;
    }


} // namespace golias
