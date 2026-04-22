#include "Graphics/PostProcess/PostProcessPass.h"

#include "Core/DShader.h"
#include "Reflection/ReflectionRegistry.h"

DELTA_ENGINE_NS_BEGIN

PostProcessPass::~PostProcessPass()
{
    ReleaseFallbackShader();
}

DShader* PostProcessPass::ResolveShader(const std::wstring& fallbackPath,
                                        const std::wstring& vsEntry,
                                        const std::wstring& psEntry,
                                        const std::wstring& vsProfile,
                                        const std::wstring& psProfile)
{
    if (m_shader)
        return m_shader;

    if (!m_fallbackShader)
    {
        m_fallbackShader = CreateDObject<DShader>();
        m_fallbackShader->Initialize(fallbackPath, vsEntry, psEntry, vsProfile, psProfile);
    }
    return m_fallbackShader;
}

void PostProcessPass::ReleaseFallbackShader()
{
    if (m_fallbackShader)
    {
        GetReflectionRegistry().DestroyObject(m_fallbackShader);
        m_fallbackShader = nullptr;
    }
}

DELTA_ENGINE_NS_END
