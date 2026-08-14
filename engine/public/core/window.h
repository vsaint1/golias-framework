#pragma once
#include "stdafx.h"


namespace golias {


    class Window {
    public:
        Window(int width, int height, const String title = "Golias Framework");
        ~Window();

        void PollEvents();
        bool ShouldClose() const;
        void SetTitle(const String& title);

        int GetWidth() const;
        int GetHeight() const;

        void GetFramebufferSize(int* width, int* height) const;

        void* GetNativeHandle() const;
       
        void* GetNativeViewHandle() const;
       
        void* GetGLFWHandle() const;

        void WaitForEvents();

        std::function<void(int, int)> OnResize;

    private:
        static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

        GLFWwindow* mWindow = nullptr;
        int mWidth          = 0;
        int mHeight         = 0;
        String mTitle;
    };
} // namespace golias
