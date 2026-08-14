#pragma once
#include "stdafx.h"

namespace golias {


    class Window {
    public:
        virtual ~Window() = default;

        virtual void PollEvents()                  = 0;
        virtual bool ShouldClose() const           = 0;
        virtual void SetTitle(const String& title) = 0;

        virtual void GetFramebufferSize(int* width, int* height) const = 0;

        virtual void* GetNativeHandle() const = 0;

        virtual void* GetNativeViewHandle() const = 0;

        virtual void* GetHandle() const = 0;

        virtual void WaitForEvents() = 0;
        
        int GetWidth() const;

        int GetHeight() const;

        std::function<void(int, int)> OnResize;

    private:
        void* mWindow = nullptr; // Native window handle
        int mWidth    = 0;
        int mHeight   = 0;
        String mTitle;
    };
} // namespace golias
