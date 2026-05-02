#include "Runtime/Core/Skybox.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <filesystem>
#include <memory>
#include <string>

using namespace DeltaEngine;

Skybox::Skybox()
    : m_cubemapTexture(nullptr)
    , m_material(nullptr)
{
}

void Skybox::Initialize(std::shared_ptr<DXGraphicsContext> context)
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    if (!m_cubemapTexture)
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "Skybox::Initialize: m_cubemapTexture is null — skipping GPU initialization");
        return;
    }

    if (!m_material)
    {
        DShader* shader = CreateDObject<DShader>();
        shader->Initialize(
            std::filesystem::path("Skybox.slang"),
            "VSMain",
            "PSMain",
            "vs_6_6",
            "ps_6_6");

        m_material = CreateDObject<DMaterial>();
        m_material->Initialize(shader);
    }

    m_renderProxy = std::make_shared<SkyboxRenderProxy>(m_cubemapTexture, m_material);
    DELTA_ASSERT(m_renderProxy != nullptr);
    m_renderProxy->Initialize(context);
}

void Skybox::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (!DELTA_ENSURE(context != nullptr))
        return;

    if (!m_renderProxy)
        return;

    m_renderProxy->GatherDrawCalls(context);
}
