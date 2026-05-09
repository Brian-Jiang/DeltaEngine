#pragma once

#include "EngineIncludes.h"

#include "DirectXMath.h"
#include "SimpleMath.h"

#include "Runtime/Core/DComponent.h"
#include "Runtime/Serialization/TBulkData.h"

#include <filesystem>
#include <string>
#include <vector>

#include "ReflectionTestObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DTexture;

DSTRUCT()
struct ReflectionTestNestStruct
{
    DGENERATED_BODY_STRUCT(ReflectionTestNestStruct)

    DPROPERTY()
    int m_nestedInt = 0;

    DPROPERTY()
    float m_nestedFloat = 0.f;
};

DCLASS()
class ReflectionTestObject : public DComponent
{
    DGENERATED_BODY(ReflectionTestObject)

public:
    DFUNCTION()
    void VoidMethod();

    DFUNCTION()
    int Compute(int x);

    DFUNCTION()
    float Compute(float x);

private:
    DPROPERTY()
    float m_rFloat = 0.0f;

    DPROPERTY()
    int m_rInt = 0;

    DPROPERTY()
    bool m_rBool = false;

    DPROPERTY()
    double m_rDouble = 0.0;

    DPROPERTY()
    std::string m_rString;

    DPROPERTY()
    std::filesystem::path m_rPath;

    DPROPERTY()
    DirectX::SimpleMath::Vector3 m_rVec3;

    DPROPERTY()
    DirectX::SimpleMath::Quaternion m_rQuat;

    DPROPERTY()
    DirectX::XMFLOAT4 m_rFloat4 = {};

    DPROPERTY()
    DirectX::XMFLOAT4X4 m_rFloat4x4 = {};

    DPROPERTY()
    DTexture* m_rTexture = nullptr;

    DPROPERTY()
    std::vector<int> m_rIntVec;

    DPROPERTY()
    std::vector<DTexture*> m_rTexVec;

    DPROPERTY()
    ReflectionTestNestStruct m_rNest = {};

    DPROPERTY()
    TBulkData m_rBulk;
};

DELTA_ENGINE_NS_END
