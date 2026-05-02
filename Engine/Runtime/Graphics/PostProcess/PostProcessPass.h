#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"

#include <filesystem>
#include <string>

#include "PostProcessPass.generated.h"

struct D3D12_CPU_DESCRIPTOR_HANDLE;
typedef unsigned int UINT;

DELTA_ENGINE_NS_BEGIN

class Device;
class DShader;
struct DXGraphicsContext;

DCLASS(abstract)
class DELTAENGINE_API PostProcessPass : public DObject
{
    DGENERATED_BODY(PostProcessPass)

public:
    DPROPERTY()
    std::string m_passName;

    DPROPERTY()
    bool m_enabled = true;

    DPROPERTY()
    DShader* m_shader = nullptr;

    virtual ~PostProcessPass();

    virtual void Initialize(Device& device) = 0;
    virtual void Execute(DXGraphicsContext& ctx,
                         D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                         D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                         UINT width, UINT height) = 0;
    virtual void Shutdown() {}

protected:
    DShader* ResolveShader(const std::filesystem::path& fallbackPath,
                           const std::string& vsEntry = "VSMain",
                           const std::string& psEntry = "PSMain",
                           const std::string& vsProfile = "vs_6_0",
                           const std::string& psProfile = "ps_6_0");

    void ReleaseFallbackShader();

private:
    DShader* m_fallbackShader = nullptr;
};

DELTA_ENGINE_NS_END
