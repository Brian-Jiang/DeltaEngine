#pragma once

#include "EngineIncludes.h"

#include "SimpleMath.h"
#include "Core/Component.h"
// #include "Math/Vector3.h"

DELTA_ENGINE_NS_BEGIN

class Transform: public Component
{
public:
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Quaternion rotation;
	DirectX::SimpleMath::Vector3 scale;
	Transform *parent;
	std::vector<Transform*> children;

	DirectX::XMMATRIX modelMatrix;

	Transform();
	~Transform();
};

DELTA_ENGINE_NS_END
