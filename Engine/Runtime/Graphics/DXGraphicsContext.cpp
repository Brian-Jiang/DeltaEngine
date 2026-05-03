#include "Graphics/DXGraphicsContext.h"

#include "Graphics/DirectX/CommandList.h"
#include "Graphics/Structures/RootParameterType.h"

using namespace DeltaEngine;

void DXGraphicsContext::ApplyLightBuffersToCommandList()
{
    if (!DELTA_ENSURE(commandList))
    {
        DLOG(LogRenderer, ELogLevel::Error,
            "DXGraphicsContext::ApplyLightBuffersToCommandList skipped: commandList is null");
        return;
    }

    commandList->SetGraphicsDynamicConstantBuffer(static_cast<UINT>(RootParameterType::LightCB), LightCB { static_cast<UINT>(directionalLights.size()), static_cast<UINT>(pointLights.size()), static_cast<UINT>(spotLights.size()) });
    if (!directionalLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<UINT>(RootParameterType::DirectionalLights), directionalLights);
    }

    if (!pointLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<UINT>(RootParameterType::PointLights), pointLights);
    }

    if (!spotLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(static_cast<UINT>(RootParameterType::SpotLights), spotLights);
    }
}