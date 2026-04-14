#include "Runtime/Core/Skybox.h"

#include "Core/DMaterial.h"
#include "Core/DShader.h"
#include "Reflection/ReflectionRegistry.h"
#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"

using namespace DeltaEngine;

Skybox::Skybox()
    : m_cubemapTexture(nullptr)
    , m_material(nullptr)
{
}

void Skybox::Initialize()
{
    if (!m_cubemapTexture)
        return;

    if (!m_material)
    {
        DShader* shader = CreateDObject<DShader>();
        shader->Initialize(
            L"Skybox.hlsl",
            L"VSMain", L"PSMain",
            L"vs_6_0", L"ps_6_0");

        m_material = CreateDObject<DMaterial>();
        m_material->Initialize(shader);
    }

    m_renderProxy = std::make_shared<SkyboxRenderProxy>(m_cubemapTexture, m_material);
}

void Skybox::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (!m_renderProxy)
        return;

    m_renderProxy->GatherDrawCalls(context);
}
