#include "core/application.h"

#include "core/asset/asset_manager.h"
#include "core/io/file_system.h"
#include "core/window.h"
#include "graphics/mesh_library.h"
#include "graphics/render_resources.h"
#include "graphics/rhi/vulkan/rhi_device_vulkan.h"
#include "graphics/scene_renderer.h"
#include "scene/components/3d/camera.h"
#include "scene/components/3d/mesh_filter.h"
#include "scene/components/3d/mesh_renderer.h"
#include "scene/scene.h"

namespace golias {

    void Application::Run() {
        auto window = std::make_shared<Window>(mConfig.Width, mConfig.Height, mConfig.Title);

        if (mConfig.Backend == RHIBackend::Compatibility) {
            LOG_ERROR("Compatibility backend is not available yet.");
            return;
        }

        spdlog::set_level(mConfig.Debug ? spdlog::level::debug : spdlog::level::info);

        mDevice = std::make_shared<RHIDeviceVulkan>(window.get(), mConfig.Debug);

        // mDevice->SetVsyncEnabled(false);

        AssetManager::Initialize(*mDevice);

        mScene = std::make_shared<Scene>("MainScene");

        GameObject* cameraObject = mScene->AddObject<GameObject>("MainCamera");
        Camera* camera           = cameraObject->AddComponent<Camera>();
        cameraObject->SetPosition({0.0f, 2.5f, -8.0f});
        camera->LookAt({0.0f, 0.0f, 0.0f});
        mScene->SetMainCamera(camera);

        auto defaultShader = AssetManager::Load<Shader>("internal/shaders/vulkan/default.spv");

        ShaderDesc vertexDesc;
        vertexDesc.stage             = ShaderStage::Vertex;
        vertexDesc.code              = defaultShader->GetCompiledBinary().data();
        vertexDesc.size              = defaultShader->GetCompiledBinary().size();
        vertexDesc.entrypoint        = "vertex_main";
        vertexDesc.numUniformBuffers = 3;
        ShaderHandle vertexShader    = mDevice->CreateShader(vertexDesc);

        ShaderDesc fragmentDesc;
        fragmentDesc.stage          = ShaderStage::Fragment;
        fragmentDesc.code           = defaultShader->GetCompiledBinary().data();
        fragmentDesc.size           = defaultShader->GetCompiledBinary().size();
        fragmentDesc.entrypoint     = "fragment_main";
        fragmentDesc.numSamplers    = 1;
        ShaderHandle fragmentShader = mDevice->CreateShader(fragmentDesc);

        VertexBufferDesc vertexBuffer{0, 32, false, 0};
        VertexAttribute attributes[] = {
            {0, 0, 0,  VertexElementFormat::Float3},
            {1, 0, 12, VertexElementFormat::Float3},
            {2, 0, 24, VertexElementFormat::Float2},
        };

        VertexInputState vertexInput{&vertexBuffer, 1, attributes, 3};

        ColorTargetDesc target = {
            .format = TextureFormat::B8G8R8A8_UNORM,
        };


        GraphicsPipelineTargetInfo targetInfo{&target, 1, true, TextureFormat::D32_FLOAT};

        GraphicsPipelineDesc pipelineDesc;
        pipelineDesc.vertexShader                       = vertexShader;
        pipelineDesc.fragmentShader                     = fragmentShader;
        pipelineDesc.vertexInput                        = vertexInput;
        pipelineDesc.targetInfo                         = targetInfo;
        pipelineDesc.depthStencilState.enableDepthTest  = true;
        pipelineDesc.depthStencilState.enableDepthWrite = true;
        GraphicsPipelineHandle samplePipeline           = mDevice->CreateGraphicsPipeline(pipelineDesc);

        auto create_material = [&](const glm::vec4& color) {
            auto material      = std::make_shared<Material>();
            material->Pipeline = samplePipeline;
            material->DefineTextureProperty(*mDevice, "BaseMap", 0);
            auto instance    = std::make_shared<MaterialInstance>();
            instance->Parent = material;
            instance->Color  = color;
            return instance;
        };

        auto add_primitive = [&](const String& name, const glm::vec3& position, const glm::vec4& color, Ref<Mesh> mesh) {
            GameObject* object = mScene->AddObject<GameObject>(name);
            object->SetPosition(position);
            object->AddComponent<MeshFilter>()->SetMesh(mesh);
            object->AddComponent<MeshRenderer>()->SetMaterial(create_material(color));
        };

        add_primitive("RedCube", {-3.5f, 0.0f, 1.0f}, {0.8f, 0.1f, 0.1f, 1.0f}, MeshLibrary::CreateCube(mDevice));
        add_primitive("GreenSphere", {0.0f, 0.0f, 1.0f}, {0.1f, 0.8f, 0.1f, 1.0f}, MeshLibrary::CreateSphere(mDevice));
        add_primitive("BlueCylinder", {3.5f, 0.0f, 1.0f}, {0.1f, 0.1f, 0.8f, 1.0f}, MeshLibrary::CreateCylinder(mDevice));
        add_primitive("YellowCapsule", {-2.0f, 0.0f, 4.0f}, {0.9f, 0.8f, 0.1f, 1.0f}, MeshLibrary::CreateCapsule(mDevice));
        add_primitive("PurpleTorus", {2.0f, 0.0f, 4.0f}, {0.7f, 0.2f, 0.8f, 1.0f}, MeshLibrary::CreateTorus(mDevice));
        add_primitive("WhiteQuad", {0.0f, 1.5f, 5.0f}, {0.9f, 0.9f, 0.9f, 1.0f}, MeshLibrary::CreateQuad(mDevice));

        GameObject* ground = mScene->AddObject<GameObject>("Ground");
        ground->SetScale({8.0f, 1.0f, 8.0f});
        ground->SetPosition({0.0f, -1.0f, 3.0f});
        ground->AddComponent<MeshFilter>()->SetMesh(MeshLibrary::CreatePlane(mDevice));
        auto groundMaterial = create_material({1.0f, 1.0f, 1.0f, 1.0f});

        auto checker = AssetManager::Load<Texture2D>("textures/checker.png");
        groundMaterial->SetTexture2D("BaseMap", checker);

        ground->AddComponent<MeshRenderer>()->SetMaterial(groundMaterial);

        mRenderer = std::make_shared<SceneRenderer>();
        mRenderer->Initialize(mDevice);

        TextureDesc depthDesc;
        depthDesc.width            = mConfig.Width;
        depthDesc.height           = mConfig.Height;
        depthDesc.format           = TextureFormat::D32_FLOAT;
        depthDesc.usage            = TextureUsage::DepthTarget;
        TextureHandle depthTexture = mDevice->CreateTexture(depthDesc);

        auto fpsStart      = std::chrono::steady_clock::now();
        uint32_t fpsFrames = 0;
        double fpsElapsed  = 0.0;

        while (!window->ShouldClose()) {
            window->PollEvents();


            if (GameObject* cube = mScene->FindObject<GameObject>("RedCube")) {
                cube->RotateLocal({0.0f, 1.0f, 0.0f}, 0.5f);
            }

            CommandBufferHandle commandBuffer = mDevice->BeginCommandBuffer();
            
            TextureHandle swapchainTexture;
            uint32_t width  = 0;
            uint32_t height = 0;
            if (!mDevice->AcquireSwapchainTexture(commandBuffer, &swapchainTexture, &width, &height)) {
                continue;
            }

            ++fpsFrames;
            const auto now = std::chrono::steady_clock::now();
            fpsElapsed     = std::chrono::duration<double>(now - fpsStart).count();
            if (fpsElapsed >= 0.5) {
                const double fps = static_cast<double>(fpsFrames) / fpsElapsed;
                window->SetTitle(fmt::format("{} - {:.1f} FPS [{}]", mConfig.Title, fps, mDevice->IsVsyncEnabled() ? "VSync" : "Unlocked"));
                fpsFrames  = 0;
                fpsElapsed = 0.0;
                fpsStart   = now;
            }

            RenderPassColorTarget colorTarget;
            colorTarget.texture    = swapchainTexture;
            colorTarget.clearColor = {0.25f, 0.45f, 0.75f, 1.0f};

            RenderPassDesc renderPass;
            renderPass.colorTargets       = &colorTarget;
            renderPass.numColorTargets    = 1;
            renderPass.depthStencilTarget = depthTexture;

            mDevice->BeginRenderPass(commandBuffer, renderPass);

            mRenderer->BeginFrame(commandBuffer, swapchainTexture, camera->GetView(), camera->GetProjection(), glm::vec3(1.f), width, height);
            mRenderer->RenderScene(mScene);
            mRenderer->EndFrame();

            mDevice->EndRenderPass(commandBuffer);

            mDevice->SubmitCommandBuffer(commandBuffer);
        }

        AssetManager::Shutdown();
        LOG_INFO("Application shutdown complete.");
    }

} // namespace golias
