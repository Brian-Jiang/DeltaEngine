#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include "SimpleMath.h"

DELTA_ENGINE_NS_BEGIN

/// Parses the MCP rotation wire format: 4 elements = quaternion [x,y,z,w],
/// 3 elements = euler angles [pitch(X), yaw(Y), roll(Z)] in degrees.
/// Returns false and leaves 'outRotation' unchanged for any other length.
DELTAEDITOR_API bool ParseRotationValue(
    const nlohmann::json& value,
    DirectX::SimpleMath::Quaternion& outRotation);

/// Matches SceneComponent::SetLocalRotation(Vector3) — degrees, roll/pitch/yaw order.
DELTAEDITOR_API DirectX::SimpleMath::Quaternion QuaternionFromEulerDegrees(
    const DirectX::SimpleMath::Vector3& degrees);

/// Inverse of QuaternionFromEulerDegrees; the euler form accepted by ParseRotationValue.
DELTAEDITOR_API DirectX::SimpleMath::Vector3 EulerDegreesFromQuaternion(
    const DirectX::SimpleMath::Quaternion& q);

DELTA_ENGINE_NS_END
