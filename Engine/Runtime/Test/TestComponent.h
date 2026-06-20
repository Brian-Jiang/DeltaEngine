#pragma once

#include "EngineIncludes.h"

#include <string>

#include "SimpleMath.h"
#include "Runtime/Core/Delegates/DynamicDelegate.h"
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
    DELTAENGINE_API void TestFunction();

    DFUNCTION()
    DELTAENGINE_API int TestAdd(int a, int b);

    DFUNCTION()
    DELTAENGINE_API float TestMultiply(float x, bool negate);

    DFUNCTION()
    DELTAENGINE_API void OnIntEvent(int value);

    DFUNCTION()
    DELTAENGINE_API void OnTwoArgEvent(int a, float b);

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntEvent, int, value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTwoArgEvent, int, a, float, b);

DELTA_ENGINE_NS_END
