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
    class VulkanWindowSurface;
    class VulkanDevice;
    class VulkanCommandPool;
    class VulkanSwapchain;
    class VulkanRenderPass;
    class VulkanPipeline;   
    class VulkanCommandBuffer;
    class VulkanFence;
    class VulkanSemaphore;
    class VulkanBuffer;

    class Application {
    public:
        Application(const ApplicationConfig& config = {});
        virtual ~Application();

        void Run();

    private:
        bool Initialize();
        void MainLoop();
        void Shutdown();
        void RenderFrame();

        void RecordCmdBuffers();

    private:
        Ref<Window> mWindow = nullptr;
        Ref<VulkanInstance> mInstance = nullptr;
        Ref<VulkanWindowSurface> mWindowSurface = nullptr;
        Ref<VulkanDevice> mDevice = nullptr;
        Ref<VulkanCommandPool> mCommandPool = nullptr;
        Ref<VulkanSwapchain> mSwapchain = nullptr;
        Ref<VulkanRenderPass> mRenderPass = nullptr;

        Ref<VulkanBuffer> mVertexBuffer = nullptr;
        Ref<VulkanBuffer> mIndexBuffer = nullptr;
        
        Ref<VulkanPipeline> mPipeline = nullptr;

        std::vector<Ref<VulkanCommandBuffer>> mCommandBuffers;

        uint32_t mCurrentFrame = 0;
        std::vector<Ref<VulkanFence>> mInFlightFences;
        std::vector<Ref<VulkanSemaphore>> mImageAvailableSemaphores;
        std::vector<Ref<VulkanSemaphore>> mRenderFinishedSemaphores;

        ApplicationConfig mConfig = {};
    };

} // namespace golias
