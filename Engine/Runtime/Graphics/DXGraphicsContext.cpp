#include "Graphics/DXGraphicsContext.h"

#include "Graphics/DirectX/CommandList.h"
#include "Graphics/Structures/RootParameterType.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;

void DXGraphicsContext::ApplyLightBuffersToCommandList()
{
    ApplyLightBuffersToCommandList(
        static_cast<uint32_t>(RootParameterType::LightCB),
        static_cast<uint32_t>(RootParameterType::DirectionalLights),
        static_cast<uint32_t>(RootParameterType::PointLights),
        static_cast<uint32_t>(RootParameterType::SpotLights));
}

void DXGraphicsContext::ApplyLightBuffersToCommandList(uint32_t lightCbSlot,
    uint32_t directionalLightsSlot,
    uint32_t pointLightsSlot,
    uint32_t spotLightsSlot)
{
    if (!DELTA_ENSURE(commandList))
    {
        DLOG(LogRenderer, ELogLevel::Error,
            "DXGraphicsContext::ApplyLightBuffersToCommandList skipped: commandList is null");
        return;
    }

    commandList->SetGraphicsDynamicConstantBuffer(lightCbSlot, LightCB { static_cast<UINT>(directionalLights.size()), static_cast<UINT>(pointLights.size()), static_cast<UINT>(spotLights.size()) });
    if (!directionalLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(directionalLightsSlot, directionalLights);
    }

    if (!pointLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(pointLightsSlot, pointLights);
    }

    if (!spotLights.empty())
    {
        commandList->SetGraphicsDynamicStructuredBuffer(spotLightsSlot, spotLights);
    }
}