#pragma once

#include "Runtime/EngineIncludes.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>

#include "DirectX/CommandQueue.h"
#include "DirectX/UploadBuffer.h"
#include "DirectX/DescriptorAllocator.h"
#include "Structures/Light.h"
#include "Structures/Camera.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/IBL/IBLBaker.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"
#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"
#include "Runtime/Graphics/RenderResourceReleaseQueue.h"
#include "Runtime/Graphics/RenderPath.h"
#include "Runtime/Graphics/Shadow/ShadowPassManager.h"

#include <slang-com-ptr.h>

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class RenderTarget;
class DWorld;
class DTexture;
class PostProcessStack;
class PostProcessPass;
class ShadowDepthPSO;
class RenderProxy;
class PipelineStateObject;

struct PostProcessTarget
{
    std::shared_ptr<DirectX12Texture> texture;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE srv{};
};

struct FrameGraphBinding
{
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE srv{};
};

struct FrameGraphBindings
{
    std::vector<FrameGraphBinding> entries;

    void Register(RenderGraphTextureHandle handle, D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE srv);
    D3D12_CPU_DESCRIPTOR_HANDLE RtvFor(RenderGraphTextureHandle handle) const;
    D3D12_CPU_DESCRIPTOR_HANDLE SrvFor(RenderGraphTextureHandle handle) const;
};

struct FrameGraphResources
{
    RenderGraphTextureHandle sceneColor;
    RenderGraphTextureHandle sceneDepth;
    RenderGraphTextureHandle shadowDirectional;
    RenderGraphTextureHandle shadowSpot;
    RenderGraphTextureHandle shadowPoint;
    RenderGraphTextureHandle ping;
    RenderGraphTextureHandle pong;
    RenderGraphTextureHandle resolvedScene;
    RenderGraphTextureHandle finalOutput;
    RenderGraphTextureHandle gbufferAlbedo;
    RenderGraphTextureHandle gbufferNormal;
    RenderGraphTextureHandle gbufferMaterial;
    RenderGraphTextureHandle gbufferEmissive;
};

/// Renders the scene to an offscreen render target. Does not own swap chain or window.
/// Device and RenderTarget are provided externally (e.g. by Editor or game launcher).
class DXRenderManager : public std::enable_shared_from_this<DXRenderManager>
{
public:
	DELTAENGINE_API DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height);
	DELTAENGINE_API ~DXRenderManager();
	DELTAENGINE_API void LoadPipeline();
    DELTAENGINE_API void LoadAssets();

    /// Creates a command list, initialises every renderer in the world,
    /// executes the command list, and waits for the GPU.
    DELTAENGINE_API void InitWorldRenderers(DWorld& world);

    using SceneDrawCallback = std::function<void(const std::shared_ptr<DXGraphicsContext>&)>;

    /// Prepares the command list and renders the scene to m_renderTarget.
    /// Caller is responsible for executing the command list and presenting.
    DELTAENGINE_API void PrepareFrame();

    /// Renders the forward scene pass as a render graph node: clears + binds the
    /// scene render target, stages frame descriptors, then invokes drawCallback
    /// to record the scene draws. Call between PrepareFrame and RenderFrame.
    DELTAENGINE_API void RenderScene(const SceneDrawCallback& drawCallback);

    DELTAENGINE_API void RenderFrame();

    DELTAENGINE_API void SetPendingActiveRenderCamera(std::optional<ActiveRenderCamera> camera);

	DELTAENGINE_API void Resize(UINT width, UINT height);
    DELTAENGINE_API void OnDestroy();

	/// Returns a context that renderers use for InitGraphicState / GatherDrawCalls.
    DELTAENGINE_API std::shared_ptr<DXGraphicsContext> GetGraphicsContext();

	inline UINT GetWidth() const { return m_width; }
    inline UINT GetHeight() const { return m_height; }
    inline float GetAspectRatio() const { return m_aspectRatio; }

	inline std::shared_ptr<RootSignature> GetRootSignature() const { return m_rootSignature; }
    inline std::shared_ptr<RootSignature> GetGBufferRootSignature() const { return m_gbufferRootSignature; }
    inline RenderPath GetRenderPath() const { return m_renderPath; }
    DELTAENGINE_API void SetRenderPath(RenderPath path) { m_renderPath = path; }
    DELTAENGINE_API ISlangBlob* GetGBufferVertexShaderBlob() const;
    DELTAENGINE_API ISlangBlob* GetGBufferPixelShaderBlob() const;
    DELTAENGINE_API D3D12_RT_FORMAT_ARRAY GetGBufferRTVFormats() const;
    DELTAENGINE_API bool EnsureDeferredLightingPipeline();
    DELTAENGINE_API std::shared_ptr<RootSignature> GetDeferredLightingRootSignature() const
    {
        return m_deferredLightingRootSignature;
    }
    DELTAENGINE_API std::shared_ptr<PipelineStateObject> GetDeferredLightingPSO() const { return m_deferredLightingPSO; }
    inline std::shared_ptr<Device> GetDevice() const { return m_device; }
    inline std::shared_ptr<RenderTarget> GetRenderTarget() const { return m_renderTarget; }
    inline std::shared_ptr<CommandList> GetCurrentCommandList() const { return m_currentCommandList; }

    DELTAENGINE_API const ShadowDepthPSO* GetShadowDepthPSO() const;

    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetFinalSceneSRV() const { return m_finalPostProcessSRV; }
    DELTAENGINE_API bool HasPostProcessedOutput() const { return m_hasPostProcessedOutput; }
    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetFinalPostProcessTexture() const
    {
        return m_hasPostProcessedOutput ? m_finalPostProcessTexture : nullptr;
    }

    /// Enqueues a render proxy for deferred GPU release after the last submitted frame fence.
    DELTAENGINE_API RenderResourceReleaseToken DeferRenderProxyRelease(std::shared_ptr<RenderProxy> proxy);

    /// Frame-scoped texture pool shared by the render graph and the editor display path.
    DELTAENGINE_API TransientTexturePool& GetTransientPool() { return m_transientPool; }

    DELTAENGINE_API void StageIBLDescriptors(CommandList& commandList, int32_t iblRootParameterIndex);
    DELTAENGINE_API void StageShadowDescriptors(CommandList& commandList, int32_t shadowMapsRootParameter,
        int32_t shadowCbRootParameter);

private:
    void CreatePingPongTargets(UINT width, UINT height);
    void BuildFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack);
    void BuildForwardFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack);
    void BuildDeferredFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack);
    bool ImportSceneTargets(RenderGraphTextureUsage colorAndShader, RenderGraphTextureUsage depthUsage,
        std::shared_ptr<DirectX12Texture>& colorTexture, std::shared_ptr<DirectX12Texture>& depthTexture);
    void AddShadowSceneSkyboxPasses(const SceneDrawCallback& drawCallback, const float clearColor[4],
        RenderGraphTextureUsage depthAndShader);
    void AddShadowPasses(RenderGraphTextureUsage depthAndShader);
    void CreateGBufferTextures(RenderGraphTextureUsage gbufferUsage);
    void InitGBufferPipeline();
    bool InitDeferredLightingPipeline();
    void FinalizeNoPostProcessOutput();
    void AppendPostProcessChain(PostProcessStack* stack, RenderGraphTextureHandle postInputHandle,
        RenderGraphTextureUsage colorAndShader);
    void ExecuteFrameGraph(DXGraphicsContext& ctx, const SceneDrawCallback& drawCallback);
    void ExecuteBootstrapSceneFallback(DXGraphicsContext& ctx, const SceneDrawCallback& drawCallback);
    void UpdateIBL(DTexture* skyboxCubemap);
    void EnsureIBLFallback();
    void StageIBLDescriptors(CommandList& commandList);
    void StageShadowDescriptors(CommandList& commandList);

	D3D12_RECT m_scissorRect;
	CD3DX12_VIEWPORT m_viewport;

	std::shared_ptr<Device> m_device;
    std::shared_ptr<RenderTarget> m_renderTarget;
    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<RootSignature> m_gbufferRootSignature;
    Slang::ComPtr<ISlangBlob> m_gbufferVertexShaderBlob;
    Slang::ComPtr<ISlangBlob> m_gbufferPixelShaderBlob;
    std::shared_ptr<RootSignature> m_deferredLightingRootSignature;
    std::shared_ptr<PipelineStateObject> m_deferredLightingPSO;
    bool m_deferredLightingReady = false;
	std::shared_ptr<CommandList> m_currentCommandList;

    PostProcessTarget m_pingPong[2];
    std::unordered_set<PostProcessPass*> m_trackedPasses;
    D3D12_CPU_DESCRIPTOR_HANDLE m_finalPostProcessSRV{};
    bool m_hasPostProcessedOutput = false;
    std::shared_ptr<DirectX12Texture> m_finalPostProcessTexture;

    std::shared_ptr<DXGraphicsContext> m_currentContext;

    RenderGraph m_frameGraph;
    TransientTexturePool m_transientPool;
    FrameGraphResources m_frameResources;
    FrameGraphBindings m_frameBindings;
    bool m_frameGraphDirty = true;
    SceneDrawCallback m_pendingSceneDrawCallback;

    std::optional<ActiveRenderCamera> m_pendingActiveRenderCamera;

    IBLBaker m_iblBaker;
    ShadowPassManager m_shadowPass;
    IBLBaker::IBLResources m_iblResources;
    DTexture* m_lastSkyboxTexture = nullptr;
    bool m_iblFallbackReady = false;
    DWorld* m_currentWorld = nullptr;

    RenderResourceReleaseQueue m_releaseQueue;
    uint64_t m_lastSubmittedFence = 0;

    RenderPath m_renderPath;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;
};

DELTA_ENGINE_NS_END
