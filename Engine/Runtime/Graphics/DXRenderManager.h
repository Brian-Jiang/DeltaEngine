#pragma once

#include "Runtime/EngineIncludes.h"

#include <chrono>
#include <memory>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>

#include <dxcapi.h>

#include "DirectX/CommandQueue.h"
#include "DirectX/UploadBuffer.h"
#include "DirectX/DescriptorAllocator.h"
#include "Structures/Light.h"
#include "Structures/Camera.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class RenderTarget;
class DWorld;
class PostProcessStack;

struct PostProcessTarget
{
    std::shared_ptr<DirectX12Texture> texture;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE srv{};
};

/// Renders the scene to an offscreen render target. Does not own swap chain or window.
/// Device and RenderTarget are provided externally (e.g. by Editor or game launcher).
class DXRenderManager : public std::enable_shared_from_this<DXRenderManager>
{
public:
	DELTAENGINE_API DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height);
	DELTAENGINE_API void LoadPipeline();
    DELTAENGINE_API void LoadAssets();

    /// Creates a command list, initialises every renderer in the world,
    /// executes the command list, and waits for the GPU.
    DELTAENGINE_API void InitWorldRenderers(DWorld& world);

    /// Prepares the command list and renders the scene to m_renderTarget.
    /// Caller is responsible for executing the command list and presenting.
    DELTAENGINE_API void PrepareFrame();
    DELTAENGINE_API void RenderFrame();

	DELTAENGINE_API void Resize(UINT width, UINT height);
    DELTAENGINE_API void OnDestroy();

	/// Returns a context that renderers use for InitGraphicState / GatherDrawCalls.
    DELTAENGINE_API std::shared_ptr<DXGraphicsContext> GetGraphicsContext();

	inline UINT GetWidth() const { return m_width; }
    inline UINT GetHeight() const { return m_height; }
    inline float GetAspectRatio() const { return m_aspectRatio; }

	inline std::shared_ptr<RootSignature> GetRootSignature() const { return m_rootSignature; }
    inline std::shared_ptr<Device> GetDevice() const { return m_device; }
    inline std::shared_ptr<RenderTarget> GetRenderTarget() const { return m_renderTarget; }
    inline std::shared_ptr<CommandList> GetCurrentCommandList() const { return m_currentCommandList; }

    DELTAENGINE_API D3D12_CPU_DESCRIPTOR_HANDLE GetFinalSceneSRV() const { return m_finalPostProcessSRV; }

private:
    void CreatePingPongTargets(UINT width, UINT height);
    void ExecutePostProcessStack(DXGraphicsContext& ctx, PostProcessStack* stack, UINT width, UINT height);

	D3D12_RECT m_scissorRect;
	CD3DX12_VIEWPORT m_viewport;

	std::shared_ptr<Device> m_device;
    std::shared_ptr<RenderTarget> m_renderTarget;
    std::shared_ptr<RootSignature> m_rootSignature;
	std::shared_ptr<CommandList> m_currentCommandList;

    PostProcessTarget m_pingPong[2];
    Microsoft::WRL::ComPtr<IDxcBlob> m_postProcessVS;
    D3D12_CPU_DESCRIPTOR_HANDLE m_finalPostProcessSRV{};

    std::shared_ptr<DXGraphicsContext> m_currentContext;

    UINT m_width;
    UINT m_height;
    float m_aspectRatio;
};

DELTA_ENGINE_NS_END
