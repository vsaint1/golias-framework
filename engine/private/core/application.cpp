#include "core/application.h"
#include "graphics/rhi/vulkan/instance.h"
#include "graphics/rhi/vulkan/window_surface.h"
#include "graphics/rhi/vulkan/vk_device.h"

namespace golias {


    Application::Application(const ApplicationConfig& config) : mConfig(config) {
    }

    Application::~Application() {
    }

    void Application::Run() {
        if (!Initialize()) {
            LOG_ERROR("Failed to initialize application.");
            return;
        }

        MainLoop();
        Shutdown();
    }

    bool Application::Initialize() {
        if (!glfwInit()) {
            LOG_ERROR("Failed to initialize the Windowing system.");
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        mWindow = std::make_shared<Window>(mConfig.Width, mConfig.Height, mConfig.Title);

        mWindow->OnResize = [this](int width, int height) {
            mConfig.Width = width;
            mConfig.Height = height;
            LOG_INFO("Resized to {}x{}", width, height);
        };

        mInstance = std::make_shared<VulkanInstance>();

        mWindowSurface = std::make_shared<VulkanWindowSurface>(mInstance, mWindow);


        mDevice = std::make_shared<VulkanDevice>(mInstance, mWindowSurface);

        return true;
    }

    void Application::MainLoop() {
        while (!mWindow->ShouldClose()) {
            mWindow->PollEvents();
        }
    }

    void Application::Shutdown() {
        mWindow.reset();
        mInstance.reset();
        mWindowSurface.reset();
        mDevice.reset();
    }


} // namespace golias
