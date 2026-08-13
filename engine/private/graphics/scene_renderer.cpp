#include "graphics/scene_renderer.h"

#include "common.h"
#include "scene/components/3d/mesh_filter.h"
#include "scene/components/3d/mesh_renderer.h"
#include "scene/scene.h"


namespace golias {


    bool SceneRenderer::Initialize(const Ref<RHIDevice>& device) {
        mDevice = device;

        TextureDesc whiteDesc;
        whiteDesc.width      = 1;
        whiteDesc.height     = 1;
        whiteDesc.format     = TextureFormat::R8G8B8A8_UNORM;
        whiteDesc.usage      = TextureUsage::Sampler;
        mDefaultWhiteTexture = device->CreateTexture(whiteDesc);

        uint8_t white[4] = {255, 255, 255, 255};
        device->UploadToTexture(mDefaultWhiteTexture, white, 1, 1, 0);

        mDefaultSampler = device->CreateSampler({});

        return true;
    }

    void SceneRenderer::Shutdown() {
        mCommands.clear();
        mLights.clear();
        mDevice.reset();
    }

    void SceneRenderer::BeginFrame(CommandBufferHandle commandBuffer,
                                   TextureHandle renderTarget,
                                   const glm::mat4& view,
                                   const glm::mat4& projection,
                                   const glm::vec3& cameraPosition,
                                   uint32_t width,
                                   uint32_t height) {

        mCommandBuffer = commandBuffer;
        mRenderTarget  = renderTarget;

        mView       = view;
        mProjection = projection;
        mCameraPos  = cameraPosition;

        mWidth  = width;
        mHeight = height;

        mCommands.clear();
        mLights.clear();

        ResetStats();

        struct PerFrameData {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec3 cameraPosition;
        } perFrame{mView, mProjection, mCameraPos};

        if (!mPerFrameBuffer) {

            BufferDesc bufferDesc = {
                .usage = BufferUsage::Uniform,
                .size  = sizeof(PerFrameData),
            };

            mPerFrameBuffer = mDevice->CreateBuffer(bufferDesc);
        }


        mDevice->UploadToBuffer(mPerFrameBuffer, &perFrame, sizeof(perFrame), 0);
    }

    void SceneRenderer::Submit(Ref<Mesh> mesh,
                               Ref<MaterialInstance> material,
                               const glm::mat4& modelMatrix,
                               uint32_t submeshIndex,
                               bool castShadows,
                               bool receiveShadows) {

        if (!mesh || !material || !material->Parent || !mesh->VertexBuffer || !mesh->IndexBuffer || !material->Parent->Pipeline) {
            return;
        }

        mCommands.push_back({std::move(mesh), std::move(material), modelMatrix, submeshIndex, castShadows, receiveShadows});
        ++mStats.MeshCount;
    }

    void SceneRenderer::SubmitLight(const RenderLight& light) {
        mLights.push_back(light);
        ++mStats.LightCount;
    }

    void SceneRenderer::RenderScene(const Ref<Scene>& scene) {

        if (!scene) {
            return;
        }

        for (GameObject* object : scene->FindObjectsWithComponent<MeshRenderer>()) {
            MeshRenderer* renderer = object->GetComponent<MeshRenderer>();
            MeshFilter* filter     = object->GetComponent<MeshFilter>();
            if (renderer == nullptr || filter == nullptr || !renderer->IsVisible()) {
                continue;
            }

            const Ref<Mesh>& mesh                 = filter->GetMesh();
            const Ref<MaterialInstance>& material = renderer->GetMaterial();
            if (mesh == nullptr || material == nullptr) {
                continue;
            }

            for (uint32_t submesh = 0; submesh < mesh->Submeshes.size(); ++submesh) {
                Submit(mesh, material, object->GetWorldTransform(), submesh, renderer->GetCastShadows(), renderer->GetReceiveShadows());
            }
        }

        // EndFrame();
    }

    void SceneRenderer::EndFrame() {
        if (!mDevice || !mCommandBuffer) {
            return;
        }

        mDevice->SetViewport(mCommandBuffer, 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
        mDevice->SetScissor(mCommandBuffer, 0, 0, static_cast<int>(mWidth), static_cast<int>(mHeight));

        for (const RenderCommand& command : mCommands) {
            mDevice->BindGraphicsPipeline(mCommandBuffer, command.Material->Parent->Pipeline);
            for (const BoundTexture& binding : command.Material->GetTextures()) {
                if (binding.VertexStage) {
                    mDevice->BindVertexSampler(mCommandBuffer, binding.Slot, binding.Texture, binding.Sampler);
                } else {
                    mDevice->BindFragmentSampler(mCommandBuffer, binding.Slot, binding.Texture, binding.Sampler);
                }
            }


            if (!command.Material->HasTextures()) {
                mDevice->BindFragmentSampler(mCommandBuffer, ALBEDO_MAP_UNIT, mDefaultWhiteTexture, mDefaultSampler);
            }

            mDevice->BindUniformBuffer(mCommandBuffer, 0, 0, mPerFrameBuffer);

            mDevice->PushVertexUniformData(mCommandBuffer, 2, &command.ModelMatrix, sizeof(command.ModelMatrix));
            mDevice->PushFragmentUniformData(mCommandBuffer, 1, &command.Material->Color, sizeof(command.Material->Color));

            mDevice->BindVertexBuffer(mCommandBuffer, 0, command.Mesh->VertexBuffer);
            mDevice->BindIndexBuffer(mCommandBuffer, command.Mesh->IndexBuffer, command.Mesh->Indices);

            if (command.SubmeshIndex < command.Mesh->Submeshes.size()) {
                const MeshSubmesh& submesh = command.Mesh->Submeshes[command.SubmeshIndex];

                mDevice->DrawIndexed(mCommandBuffer, submesh.IndexCount, 1, submesh.FirstIndex);

                ++mStats.DrawCalls;
                mStats.TriangleCount += submesh.IndexCount / 3;
            }
        }
    }

} // namespace golias
