#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <DirectXMath.h>

#include "Runtime/Graphics/Structures/Camera.h"

DELTA_ENGINE_NS_BEGIN

/// Halton sequence sample in (0, 1) for the given base and 1-based index.
DELTAENGINE_API float Halton(uint32_t index, uint32_t base);

/// Converts Halton samples in (0,1) to NDC projection jitter for a render target size.
DELTAENGINE_API DirectX::XMFLOAT2 HaltonJitterNdc(uint32_t sampleIndex, uint32_t width, uint32_t height);

/// Applies NDC jitter to an untransposed LH perspective projection (_31/_32).
DELTAENGINE_API DirectX::XMMATRIX ApplyProjectionJitter(DirectX::FXMMATRIX unjitteredProjection,
    DirectX::XMFLOAT2 jitterNdc);

/// Fills CameraCB temporal fields from unjittered view/proj and frame jitter/prev (no state mutation).
/// Does not write cb.position.
DELTAENGINE_API void PopulateCameraCBWithTemporal(CameraCB& cb,
    DirectX::FXMMATRIX unjitteredView,
    DirectX::FXMMATRIX unjitteredProjection,
    DirectX::XMFLOAT2 jitterNdc,
    DirectX::FXMMATRIX prevUnjitteredViewProjection);

/// Per-manager temporal camera jitter state (Halton, prev unjittered VP, history reset).
struct TemporalCameraState
{
    bool enabled = false;
    uint32_t haltonIndex = 0;
    DirectX::XMFLOAT2 currentJitter{ 0.f, 0.f };
    DirectX::XMMATRIX prevUnjitteredViewProjection = DirectX::XMMatrixIdentity();
    bool historyReset = true;

    void RequestHistoryReset() { historyReset = true; }

    /// Advances Halton jitter for this frame from render-target size.
    DELTAENGINE_API void BeginFrame(uint32_t width, uint32_t height);

    /// Prev unjittered VP to bind this frame (equals current VP when historyReset).
    DELTAENGINE_API DirectX::XMMATRIX ResolvePrevUnjitteredViewProjection(
        DirectX::FXMMATRIX currentUnjitteredViewProjection) const;

    /// Stores current unjittered VP for the next frame and clears historyReset.
    DELTAENGINE_API void CommitUnjitteredViewProjection(
        DirectX::FXMMATRIX currentUnjitteredViewProjection);

    /// Writes jittered CameraCB and commits prev-unjittered-VP history.
    /// Preserves cb.position.
    DELTAENGINE_API void FillCameraCB(CameraCB& cb,
        DirectX::FXMMATRIX unjitteredView,
        DirectX::FXMMATRIX unjitteredProjection);
};

DELTA_ENGINE_NS_END
