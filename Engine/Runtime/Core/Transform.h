#pragma once

#include "SimpleMath.h"
#include "Core/Component.h"
// #include "Math/Vector3.h"

namespace DeltaEngine
{

class Transform: public Component
{
public:
	DirectX::SimpleMath::Vector3 position;
	DirectX::SimpleMath::Quaternion rotation;
	DirectX::SimpleMath::Vector3 scale;

	Transform();
	~Transform();

};

}
