#pragma once
#include "window.h"


namespace golias {

    enum class BackendType { Vulkan, OpenGL, DirectX, Metal, Auto };

    struct ApplicationConfig {
        int Width           = 1280;
        int Height          = 720;
        std::string Title   = "Golias Framework";
        BackendType Backend = BackendType::Auto;
        bool Vsync   = true;
        int MaxFPS  = 60;
    };

    class VulkanInstance;
    
    class Application {
    public:
        Application(const ApplicationConfig& config = {});
        virtual ~Application();

        void Run();

    private:
        bool Initialize();
        void MainLoop();
        void Shutdown();

    private:
        Ref<Window> mWindow = nullptr;
        Ref<VulkanInstance> mInstance = nullptr;
        ApplicationConfig mConfig = {};
    };

} // namespace golias
