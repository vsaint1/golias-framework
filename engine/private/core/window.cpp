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

    Window::Window(int width, int height, const String title) : mWidth(width), mHeight(height), mTitle(title) {
        if (!glfwInit()) {
            LOG_FATAL("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

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

    void Window::SetTitle(const String& title) {
        mTitle = title;
        if (mWindow) {
            glfwSetWindowTitle(mWindow, mTitle.c_str());
        }
    }

    void Window::GetFramebufferSize(int* width, int* height) const {
        if (mWindow) {
            glfwGetFramebufferSize(mWindow, width, height);
        } else {
            *width  = 0;
            *height = 0;
        }
    }

    void Window::WaitForEvents() {
        glfwWaitEvents();
    }
    
    std::vector<const char*> Window::GetRequiredInstanceExtensions() const {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        if (!extensions || count == 0) {
            LOG_ERROR("GLFW did not provide Vulkan instance extensions.");
            return {};
        }

        return std::vector<const char*>(extensions, extensions + count);
    }

    VkResult Window::CreateSurface(VkInstance instance, VkSurfaceKHR* surface) const {
        if (!mWindow) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return glfwCreateWindowSurface(instance, mWindow, nullptr, surface);
    }


    int Window::GetWidth() const {
        return mWidth;
    }

    int Window::GetHeight() const {
        return mHeight;
    }


} // namespace golias
