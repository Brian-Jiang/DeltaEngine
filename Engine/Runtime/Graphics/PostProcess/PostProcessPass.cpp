#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include "Runtime/Core/DShader.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

DELTA_ENGINE_NS_BEGIN

PostProcessPass::~PostProcessPass()
{
    ReleaseFallbackShader();
}

DShader* PostProcessPass::ResolveShader(const std::filesystem::path& fallbackPath,
                                        const std::string& vsEntry,
                                        const std::string& psEntry,
                                        const std::string& vsProfile,
                                        const std::string& psProfile)
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
