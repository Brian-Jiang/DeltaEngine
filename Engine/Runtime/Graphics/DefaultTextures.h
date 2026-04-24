#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class DirectX12Texture;

/// Lazily creates (and caches) engine-wide fallback textures used when a material
/// slot has no texture bound. Must be initialized with a valid Device + CommandList
/// pair (the command list is used to upload the initial pixel data).
class DefaultTextures
{
public:
    /// Creates the default 1x1 white texture if it does not exist yet.
    /// Safe to call once during DXRenderManager::LoadAssets / InitWorldRenderers.
    DELTAENGINE_API static void Initialize(Device& device, CommandList& commandList);

    /// Returns the CPU descriptor handle of the default white texture SRV.
    /// Returns a zero-initialised handle when Initialize has not been called.
    DELTAENGINE_API static D3D12_CPU_DESCRIPTOR_HANDLE GetWhiteSRV();

    /// Returns the underlying texture (may be null if not yet initialized).
    DELTAENGINE_API static std::shared_ptr<DirectX12Texture> GetWhiteTexture();

    /// Releases any cached textures. Call during renderer shutdown.
    DELTAENGINE_API static void Shutdown();
};

DELTA_ENGINE_NS_END
