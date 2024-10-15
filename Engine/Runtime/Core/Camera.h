#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "SimpleMath.h"
#include "Core/Object.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/Texture.h"
#include "Core/Component.h"

DELTA_ENGINE_NS_BEGIN

class Camera : public Component
{
public:
    float fov;

    Camera();
    ~Camera();

    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;

    void Start(DirectX::XMFLOAT3 forward, DirectX::XMFLOAT3 up, float fov, float aspectRatio, float nearPlane, float farPlane);
    void Tick();
};

DELTA_ENGINE_NS_END
