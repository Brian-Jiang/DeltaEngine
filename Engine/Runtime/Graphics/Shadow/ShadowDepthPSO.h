#pragma once

#include "EngineIncludes.h"

#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;
class PipelineStateObject;
class RootSignature;

namespace ShadowDepthRS
{
enum : uint32_t
{
    ViewProjCB = 0,
    WorldMatrixCB = 1,
    NumParameters = 2
};
}

class ShadowDepthPSO
{
public:
    DELTAENGINE_API explicit ShadowDepthPSO(Device& device);

    DELTAENGINE_API std::shared_ptr<RootSignature> GetRootSignature() const { return m_rootSignature; }
    DELTAENGINE_API std::shared_ptr<PipelineStateObject> GetPSO2D() const { return m_pso2D; }
    DELTAENGINE_API std::shared_ptr<PipelineStateObject> GetPSOCube() const { return m_psoCube; }

private:
    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<PipelineStateObject> m_pso2D;
    std::shared_ptr<PipelineStateObject> m_psoCube;
};

DELTA_ENGINE_NS_END
