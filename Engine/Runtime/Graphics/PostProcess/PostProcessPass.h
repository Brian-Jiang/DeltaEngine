#pragma once

#include "EngineIncludes.h"

#include "Core/DObject.h"

#include <string>

#include "PostProcessPass.generated.h"

struct D3D12_CPU_DESCRIPTOR_HANDLE;
typedef unsigned int UINT;

DELTA_ENGINE_NS_BEGIN

class Device;
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

    virtual void Initialize(Device& device) = 0;
    virtual void Execute(DXGraphicsContext& ctx,
                         D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                         D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                         UINT width, UINT height) = 0;
};

DELTA_ENGINE_NS_END
