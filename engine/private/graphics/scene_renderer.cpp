#include "graphics/scene_renderer.h"

#include "common.h"
#include "scene/components/3d/mesh_filter.h"
#include "scene/components/3d/mesh_renderer.h"
#include "scene/scene.h"



namespace golias {


    bool SceneRenderer::Initialize(const Ref<RHIDevice>& device) {
        mDevice        = device;
        mPipelineCache = std::make_unique<GraphicsPipelineStateCache>(device);

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
        if (mPipelineCache) {
            mPipelineCache->Clear();
        }

        mDevice.reset();
        mPipelineCache.reset();
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

        if (!mesh || !material || !material->Parent || !material->Parent->GetShader() || !mesh->VertexBuffer || !mesh->IndexBuffer) {
            return;
        }

        const BlendMode blendMode = material->GetBlendMode();
        mCommands.push_back({std::move(mesh), std::move(material), modelMatrix, submeshIndex, castShadows, receiveShadows, blendMode});
        ++mStats.MeshCount;
    }

    bool SceneRenderer::IsTransparent(const Ref<MaterialInstance>& material) const {
        if (!material || !material->Parent) {
            return false;
        }

        return material->GetBlendMode() != BlendMode::Opaque;
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

    void SceneRenderer::SortCommands() {

        std::stable_sort(mCommands.begin(), mCommands.end(), [this](const RenderCommand& a, const RenderCommand& b) {
            const bool aTransparent = a.MaterialBlendMode != BlendMode::Opaque;
            const bool bTransparent = b.MaterialBlendMode != BlendMode::Opaque;
            if (aTransparent != bTransparent) {
                return !aTransparent;
            }

            if (!aTransparent) {
                return false;
            }

            const glm::vec3 aPosition = glm::vec3(a.ModelMatrix[3]);
            const glm::vec3 bPosition = glm::vec3(b.ModelMatrix[3]);
            return glm::length2(aPosition - mCameraPos) > glm::length2(bPosition - mCameraPos);
        });

    }

    void SceneRenderer::EndFrame() {
        if (!mDevice || !mCommandBuffer) {
            return;
        }

        mDevice->SetViewport(mCommandBuffer, 0.0f, 0.0f, static_cast<float>(mWidth), static_cast<float>(mHeight));
        mDevice->SetScissor(mCommandBuffer, 0, 0, static_cast<int>(mWidth), static_cast<int>(mHeight));

        SortCommands();

        for (const RenderCommand& command : mCommands) {
            const Ref<Shader>& shader = command.Material->Parent->GetShader();

            const bool transparent = command.MaterialBlendMode != BlendMode::Opaque;
            ColorTargetBlendState blendState = {
                .enableBlend    = transparent,
                .srcColorFactor = command.MaterialBlendMode == BlendMode::Premultiplied ? BlendFactor::One : BlendFactor::SrcAlpha,
                .dstColorFactor = command.MaterialBlendMode == BlendMode::Opaque
                                      ? BlendFactor::Zero
                                      : (command.MaterialBlendMode == BlendMode::Additive ? BlendFactor::One : BlendFactor::OneMinusSrcAlpha),
                .colorOp        = BlendOp::Add,
                .srcAlphaFactor = BlendFactor::One,
                .dstAlphaFactor = transparent ? BlendFactor::OneMinusSrcAlpha : BlendFactor::Zero,
                .alphaOp        = BlendOp::Add,
            };

            PipelineKey key = {
                .VertexShader     = shader->GetHandle(ShaderStage::Vertex),
                .FragmentShader   = shader->GetHandle(ShaderStage::Fragment),
                .VertexBuffers    = command.Mesh->VertexBuffers,
                .VertexAttributes = command.Mesh->VertexAttributes,
                .Rasterizer       = {.cullMode = command.Material->GetCullMode()},
                .DepthStencil     = {.enableDepthTest = command.Material->GetDepthTest(),
                                     .enableDepthWrite = command.Material->GetDepthWrite(),
                                     .enableStencilTest = true},
                .BlendState       = blendState,
                .ColorFormat      = mDevice->GetSwapchainFormat(),
                .DepthFormat      = mDevice->GetDepthFormat(),
            };

            mDevice->BindGraphicsPipeline(mCommandBuffer, mPipelineCache->TryGet(key));

            for (const auto& entry : shader->GetProperties()) {
                const ShaderProperty& property     = entry.second;
                const MaterialPropertyValue* value = command.Material->Resolve(property.Id);

                if (property.Type == ShaderPropertyType::Texture2D) {
                    TextureHandle texture = mDefaultWhiteTexture;
                    if (value && std::holds_alternative<Ref<Texture2D>>(*value) && std::get<Ref<Texture2D>>(*value)) {
                        texture = std::get<Ref<Texture2D>>(*value)->GetHandle();
                    }

                    if (property.Stage == ShaderStage::Vertex) {
                        mDevice->BindVertexSampler(mCommandBuffer, property.Binding, texture, mDefaultSampler);
                    } else {
                        mDevice->BindFragmentSampler(mCommandBuffer, property.Binding, texture, mDefaultSampler);
                    }

                } else if (value && std::holds_alternative<glm::vec4>(*value)) {
                    mDevice->PushFragmentUniformData(mCommandBuffer, property.Binding, &std::get<glm::vec4>(*value), sizeof(glm::vec4));
                } else if (value && std::holds_alternative<glm::vec3>(*value)) {
                    mDevice->PushFragmentUniformData(mCommandBuffer, property.Binding, &std::get<glm::vec3>(*value), sizeof(glm::vec3));
                } else if (value && std::holds_alternative<glm::vec2>(*value)) {
                    mDevice->PushFragmentUniformData(mCommandBuffer, property.Binding, &std::get<glm::vec2>(*value), sizeof(glm::vec2));
                } else if (value && std::holds_alternative<float>(*value)) {
                    mDevice->PushFragmentUniformData(mCommandBuffer, property.Binding, &std::get<float>(*value), sizeof(float));
                }
            }

            mDevice->BindUniformBuffer(mCommandBuffer, 0, 0, mPerFrameBuffer);


            mDevice->PushVertexUniformData(mCommandBuffer, 2, &command.ModelMatrix, sizeof(command.ModelMatrix));

            mDevice->BindVertexBuffer(mCommandBuffer, 0, command.Mesh->VertexBuffer);

            mDevice->BindIndexBuffer(mCommandBuffer, command.Mesh->IndexBuffer, command.Mesh->Indices);

            if (command.SubmeshIndex < command.Mesh->Submeshes.size()) {
                const MeshSubmesh& submesh = command.Mesh->Submeshes[command.SubmeshIndex];

                mDevice->DrawIndexed(mCommandBuffer, submesh.IndexCount, 1, submesh.FirstIndex);

                ++mStats.DrawCalls;
                mStats.TriangleCount += submesh.IndexCount / 3;
            }
        }
    } // namespace golias

} // namespace golias
