#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>

#include "Graphics/DXRenderManager.h"
#include "Graphics/Renderer/SpriteRenderer.h"
#include "Graphics/Renderer/MeshRenderer.h"
#include "Graphics/DirectX/InstancedDrawer.h"
#include "Runtime/Core/DWorld.h"
#include "SDL3/SDL.h"
#include "Core/Time.h"

DELTA_ENGINE_NS_BEGIN

enum class GameState {
	PLAY,
	EXIT,
	Error,
};

class EngineMain
{
public:
	EngineMain();

	void Initialize();
	void StartMainLoop();

	int exitCode;

	DXRenderManager* dxRenderManager;

	static EngineMain* instance;

private:
	void InitSDL();
	void HandleInput();
	void Draw();

	SDL_Renderer* renderer;
	SDL_Window* window;

	GameState gameState;

	MeshRenderer *meshRenderer;
	MeshRenderer *meshRenderer2;
	InstancedDrawer* m_instancedDrawer;

	float m_FoV;

    Time* time;
	
public:
	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;
	DirectX::XMVECTOR eyePosition;
	DirectX::XMMATRIX mvpMatrix;

private:
    std::shared_ptr<DWorld> m_world;

};

DELTA_ENGINE_NS_END
