#include "Graphics/Structures/TemporalCameraState.h"

#include <cmath>

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

float DeltaEngine::TemporalTextureMipBias(float upscaleFactor)
{
    if (upscaleFactor > 1.f)
        return -std::log2(upscaleFactor);
    return kNativeTemporalMipBias;
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
    cb.mipBias = 0.f;
    cb.cameraPad = 0.f;
    PopulateInvViewProjection(cb);
}

namespace
{
void ExtractCameraPose(FXMMATRIX unjitteredView, XMFLOAT3& outPosition, XMFLOAT3& outForward)
{
    const XMMATRIX cameraWorld = XMMatrixInverse(nullptr, unjitteredView);
    XMStoreFloat3(&outPosition, cameraWorld.r[3]);
    XMStoreFloat3(&outForward, XMVector3Normalize(cameraWorld.r[2]));
}
} // namespace

void TemporalCameraState::SetEnabled(bool value)
{
    if (enabled == value)
        return;
    enabled = value;
    RequestHistoryReset();
}

bool TemporalCameraState::DetectCut(FXMMATRIX unjitteredView, TemporalCameraSource source) const
{
    if (prevSource == TemporalCameraSource::None)
        return false;
    if (source != prevSource)
        return true;

    XMFLOAT3 position, forward;
    ExtractCameraPose(unjitteredView, position, forward);

    const XMVECTOR delta = XMVectorSubtract(XMLoadFloat3(&position), XMLoadFloat3(&prevCameraPosition));
    if (XMVectorGetX(XMVector3Length(delta)) > cutDistanceThreshold)
        return true;

    const float cosAngle = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&forward), XMLoadFloat3(&prevCameraForward)));
    return cosAngle < std::cos(cutAngleThresholdRadians);
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
    DirectX::FXMMATRIX currentUnjitteredViewProjection,
    DirectX::CXMMATRIX unjitteredView,
    TemporalCameraSource source)
{
    prevUnjitteredViewProjection = currentUnjitteredViewProjection;
    ExtractCameraPose(unjitteredView, prevCameraPosition, prevCameraForward);
    prevSource = source;
    historyReset = false;
}

void TemporalCameraState::FillCameraCB(CameraCB& cb,
    DirectX::FXMMATRIX unjitteredView,
    DirectX::CXMMATRIX unjitteredProjection,
    TemporalCameraSource source)
{
    if (!historyReset && DetectCut(unjitteredView, source))
        RequestHistoryReset();
    historyResetThisFrame = historyReset;

    const XMVECTOR position = cb.position;
    const XMMATRIX currentUnjitteredVP = XMMatrixMultiply(unjitteredView, unjitteredProjection);
    const XMMATRIX prevForFrame = ResolvePrevUnjitteredViewProjection(currentUnjitteredVP);
    const XMFLOAT2 jitter = enabled ? currentJitter : XMFLOAT2{ 0.f, 0.f };

    PopulateCameraCBWithTemporal(cb, unjitteredView, unjitteredProjection, jitter, prevForFrame);
    cb.position = position;
    cb.mipBias = enabled ? TemporalTextureMipBias(1.f) : 0.f;

    CommitUnjitteredViewProjection(currentUnjitteredVP, unjitteredView, source);
}
