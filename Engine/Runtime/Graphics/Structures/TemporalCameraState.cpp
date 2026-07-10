#include "Graphics/Structures/TemporalCameraState.h"

using namespace DeltaEngine;
using namespace DirectX;

float DeltaEngine::Halton(uint32_t index, uint32_t base)
{
    float f = 1.0f;
    float result = 0.0f;
    uint32_t i = index;
    while (i > 0)
    {
        f /= static_cast<float>(base);
        result += f * static_cast<float>(i % base);
        i /= base;
    }
    return result;
}

DirectX::XMFLOAT2 DeltaEngine::HaltonJitterNdc(uint32_t sampleIndex, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return { 0.f, 0.f };

    constexpr uint32_t kPeriod = 8;
    const uint32_t index = (sampleIndex % kPeriod) + 1; // Halton is typically 1-based
    const float h2 = Halton(index, 2);
    const float h3 = Halton(index, 3);
    const float jx = (h2 - 0.5f) * 2.0f / static_cast<float>(width);
    const float jy = (h3 - 0.5f) * 2.0f / static_cast<float>(height);
    return { jx, jy };
}

DirectX::XMMATRIX DeltaEngine::ApplyProjectionJitter(DirectX::FXMMATRIX unjitteredProjection,
    DirectX::XMFLOAT2 jitterNdc)
{
    XMMATRIX proj = unjitteredProjection;
    proj.r[2] = XMVectorAdd(proj.r[2], XMVectorSet(jitterNdc.x, jitterNdc.y, 0.f, 0.f)); // _31, _32
    return proj;
}

void DeltaEngine::PopulateCameraCBWithTemporal(CameraCB& cb,
    DirectX::FXMMATRIX unjitteredView,
    DirectX::FXMMATRIX unjitteredProjection,
    DirectX::XMFLOAT2 jitterNdc,
    DirectX::FXMMATRIX prevUnjitteredViewProjection)
{
    const XMMATRIX jitteredProjection = ApplyProjectionJitter(unjitteredProjection, jitterNdc);

    cb.viewMatrix = XMMatrixTranspose(unjitteredView);
    cb.projectionMatrix = XMMatrixTranspose(jitteredProjection);
    cb.projectionMatrixUnjittered = XMMatrixTranspose(unjitteredProjection);
    cb.prevViewProjectionMatrix = XMMatrixTranspose(prevUnjitteredViewProjection);
    cb.jitter = jitterNdc;
    cb.jitterPad = { 0.f, 0.f };
    PopulateInvViewProjection(cb);
}

void TemporalCameraState::BeginFrame(uint32_t width, uint32_t height)
{
    if (!enabled || width == 0 || height == 0)
    {
        currentJitter = { 0.f, 0.f };
        return;
    }

    currentJitter = HaltonJitterNdc(haltonIndex, width, height);
    ++haltonIndex;
}

DirectX::XMMATRIX TemporalCameraState::ResolvePrevUnjitteredViewProjection(
    DirectX::FXMMATRIX currentUnjitteredViewProjection) const
{
    if (historyReset)
        return currentUnjitteredViewProjection;
    return prevUnjitteredViewProjection;
}

void TemporalCameraState::CommitUnjitteredViewProjection(
    DirectX::FXMMATRIX currentUnjitteredViewProjection)
{
    prevUnjitteredViewProjection = currentUnjitteredViewProjection;
    historyReset = false;
}

void TemporalCameraState::FillCameraCB(CameraCB& cb,
    DirectX::FXMMATRIX unjitteredView,
    DirectX::FXMMATRIX unjitteredProjection)
{
    const XMVECTOR position = cb.position;
    const XMMATRIX currentUnjitteredVP = XMMatrixMultiply(unjitteredView, unjitteredProjection);
    const XMMATRIX prevForFrame = ResolvePrevUnjitteredViewProjection(currentUnjitteredVP);
    const XMFLOAT2 jitter = enabled ? currentJitter : XMFLOAT2{ 0.f, 0.f };

    PopulateCameraCBWithTemporal(cb, unjitteredView, unjitteredProjection, jitter, prevForFrame);
    cb.position = position;

    CommitUnjitteredViewProjection(currentUnjitteredVP);
}
