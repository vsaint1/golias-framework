#pragma once
#include "core/window.h"
#include <glfw/glfw3.h>

namespace golias {


    class GLFW_Window final : public Window {
    public:


        GLFW_Window(int width, int height, const String title = "Golias Framework");
        ~GLFW_Window() override;

        void PollEvents() override;
        bool ShouldClose() const override;
        void SetTitle(const String& title) override;

        void GetFramebufferSize(int* width, int* height) const override;

        void* GetNativeHandle() const override;
       
        void* GetNativeViewHandle() const override;
       
        void* GetHandle() const override;

        void WaitForEvents() override;

        std::function<void(int, int)> OnResize;

    private:
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

        GLFWwindow* mWindow = nullptr;
        int mWidth          = 0;
        int mHeight         = 0;
        String mTitle;
    };
} // namespace golias
