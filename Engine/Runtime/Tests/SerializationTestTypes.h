#pragma once

#include "EngineIncludes.h"

#include <string>

#include "SimpleMath.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include "SerializationTestTypes.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DTestObjectA : public DObject
{
    DGENERATED_BODY(DTestObjectA)

    DPROPERTY()
    float m_health = 100.0f;

    DPROPERTY()
    int m_level = 1;

    DPROPERTY()
    bool m_isActive = false;

    DPROPERTY()
    double m_stamina = 50.0;

    DPROPERTY()
    std::string m_name = "DefaultName";

    DPROPERTY()
    DirectX::SimpleMath::Vector3 m_position = {};

    DPROPERTY()
    DirectX::SimpleMath::Quaternion m_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
};


DCLASS()
class DTestObjectB : public DObject
{
    DGENERATED_BODY(DTestObjectB)

    DPROPERTY()
    std::string m_label = "Ref";

    DPROPERTY()
    float m_weight = 1.0f;

    DPROPERTY()
    DTestObjectA* m_targetRef = nullptr;
};


DCLASS()
class PA_TestAsset : public DPrimaryAsset
{
    DGENERATED_BODY(PA_TestAsset)
};

DELTA_ENGINE_NS_END
