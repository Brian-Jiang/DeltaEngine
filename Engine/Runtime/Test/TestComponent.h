#pragma once

#include "EngineIncludes.h"

#include <string>

#include "SimpleMath.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "Runtime/Test/TestComponent.generated.h"


DELTA_ENGINE_NS_BEGIN

class DTexture;

DCLASS()
class TestComponent : public DObject
{
    DGENERATED_BODY(TestComponent)

public:
    DFUNCTION()
    void TestFunction();

    DFUNCTION()
    int TestAdd(int a, int b);

    DFUNCTION()
    float TestMultiply(float x, bool negate);

private:
    DPROPERTY()
    float m_testFloat = 0.0f;

    DPROPERTY()
    int m_testInt = 0;

    DPROPERTY()
    bool m_testBool = false;

    DPROPERTY()
    double m_testDouble = 0.0;

    DPROPERTY()
    std::string m_testString;

    DPROPERTY()
    DirectX::SimpleMath::Vector3 m_testVector3;

    DPROPERTY()
    DirectX::SimpleMath::Quaternion m_testQuaternion;

    DPROPERTY()
    DTexture* m_texture = nullptr;
};

DELTA_ENGINE_NS_END
