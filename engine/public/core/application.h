#pragma once

#include "graphics/rhi/rhi_device.h"

namespace golias {

    class Scene;
    class SceneRenderer;

    struct ApplicationConfig {
        int Width          = 1280;
        int Height         = 720;
        std::string Title  = "Golias Framework";
        RHIBackend Backend = RHIBackend::Auto;
        bool Vsync         = true;
        bool Debug         = false;
        int MaxFPS         = 60;
    };

    class Application {
    public:
        explicit Application(const ApplicationConfig& config = {}) : mConfig(config) {
        }

        virtual ~Application() = default;
        
        void Run();

    private:
        ApplicationConfig mConfig;
        Ref<RHIDevice> mDevice;
        Ref<Scene> mScene;
        Ref<SceneRenderer> mRenderer;
    };

} // namespace golias
