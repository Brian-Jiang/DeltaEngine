#pragma once

#include "EngineIncludes.h"

#include <string>

#include "SimpleMath.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "TestComponent.generated.h"


DELTA_ENGINE_NS_BEGIN

class DTexture;

DCLASS()
class TestComponent : public DComponent
{
    DGENERATED_BODY(TestComponent)

public:
    DFUNCTION(ShowAsButton)
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

    DPROPERTY(EditorOnly)
    float m_editorOnlyFloat = 0.0f;

    DPROPERTY(HideInDetails)
    float m_hideInDetailsFloat = 0.0f;

    DPROPERTY(EditorOnly, HideInDetails)
    float m_editorOnlyAndHiddenFloat = 0.0f;
};

DELTA_ENGINE_NS_END
