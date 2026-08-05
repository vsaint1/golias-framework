#include "core/application.h"
#include "graphics/rhi/vulkan/instance.h"

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
    }


} // namespace golias
