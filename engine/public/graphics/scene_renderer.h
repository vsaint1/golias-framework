#pragma once

#include "graphics/render_resources.h"

#include <glm/glm.hpp>

namespace golias {

    class Scene;

    struct RenderCommand {
        Ref<Mesh> Mesh;
        Ref<MaterialInstance> Material;
        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        uint32_t SubmeshIndex = 0;
        bool CastShadows      = true;
        bool ReceiveShadows   = true;
    };

    struct RenderLight {
        glm::vec3 Position  = {};
        glm::vec3 Direction = {};
        glm::vec4 Color     = {1, 1, 1, 1};
        float Intensity     = 1.0f;
        float Range         = 0.0f;
        bool CastShadows    = true;
    };

    class SceneRenderer {
    public:
        struct Stats {
            uint32_t DrawCalls     = 0;
            uint32_t TriangleCount = 0;
            uint32_t MeshCount     = 0;
            uint32_t LightCount    = 0;
        };

        bool Initialize(const Ref<RHIDevice>& device);

        void Shutdown();

        void BeginFrame(CommandBufferHandle commandBuffer,
                        TextureHandle renderTarget,
                        const glm::mat4& view,
                        const glm::mat4& projection,
                        const glm::vec3& cameraPosition,
                        uint32_t width,
                        uint32_t height);

        void EndFrame();


        void Submit(Ref<Mesh> mesh,
                    Ref<MaterialInstance> material,
                    const glm::mat4& modelMatrix,
                    uint32_t submeshIndex = 0,
                    bool castShadows      = true,
                    bool receiveShadows   = true);

        void SubmitLight(const RenderLight& light);

        void RenderScene(const Ref<Scene>& scene);

        void SetEnvironmentMap(TextureHandle cubemap) {
            mEnvironmentMap = cubemap;
        }

        void SetSkyboxIntensity(float intensity) {
            mSkyboxIntensity = intensity;
        }

        void SetSkyboxVisible(bool visible) {
            mSkyboxVisible = visible;
        }

        void SetSkyboxRotation(float degrees) {
            mSkyboxRotation = degrees;
        }


        const Stats& GetStats() const {
            return mStats;
        }

        void ResetStats() {
            mStats = {};
        }

    private:
        Ref<RHIDevice> mDevice = nullptr;

        CommandBufferHandle mCommandBuffer;
        TextureHandle mRenderTarget;

        glm::mat4 mView       = glm::mat4(1.0f);
        glm::mat4 mProjection = glm::mat4(1.0f);
        glm::vec3 mCameraPos  = glm::vec3(0.0f);

        uint32_t mWidth  = 0;
        uint32_t mHeight = 0;
        TextureHandle mEnvironmentMap;

        float mSkyboxIntensity = 1.0f;
        float mSkyboxRotation  = 0.0f;
        bool mSkyboxVisible    = true;

        std::vector<RenderCommand> mCommands;
        std::vector<RenderLight> mLights;

        Stats mStats;

        BufferHandle mPerFrameBuffer;

        TextureHandle mDefaultWhiteTexture;
        SamplerHandle mDefaultSampler;
    };

} // namespace golias
