#pragma once

#include "EngineIncludes.h"

#include <vector>
#include "Core/GameObject.h"

DELTA_ENGINE_NS_BEGIN

class Scene
{
public:
	std::vector<GameObject*> gameObjects;

	Scene();
	~Scene();

	void Update();
	void Render();

};

DELTA_ENGINE_NS_END
