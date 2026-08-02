#include "Mcp/McpRotationWire.h"

#include <DirectXMath.h>

using namespace DeltaEngine;
using namespace DirectX;
using namespace DirectX::SimpleMath;

bool DeltaEngine::ParseRotationValue(const nlohmann::json& value, Quaternion& outRotation)
{
    if (!value.is_array())
        return false;

    if (value.size() == 4)
    {
        outRotation = Quaternion(value[0].get<float>(), value[1].get<float>(),
                                 value[2].get<float>(), value[3].get<float>());
        return true;
    }
    if (value.size() == 3)
    {
        outRotation = QuaternionFromEulerDegrees(
            Vector3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>()));
        return true;
    }
    return false;
}

Quaternion DeltaEngine::QuaternionFromEulerDegrees(const Vector3& degrees)
{
    return Quaternion(XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(degrees.x),
        XMConvertToRadians(degrees.y),
        XMConvertToRadians(degrees.z)));
}

Vector3 DeltaEngine::EulerDegreesFromQuaternion(const Quaternion& q)
{
    const Vector3 radians = q.ToEuler();
    return Vector3(
        XMConvertToDegrees(radians.x),
        XMConvertToDegrees(radians.y),
        XMConvertToDegrees(radians.z));
}
