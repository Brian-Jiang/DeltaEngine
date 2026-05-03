#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include "Runtime/Core/DShader.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

DELTA_ENGINE_NS_BEGIN

namespace
{
bool PostProcessShaderBlobValid(ISlangBlob* blob)
{
    return blob && blob->getBufferPointer() != nullptr && blob->getBufferSize() > 0;
}
}

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
        if (!m_fallbackShader)
        {
            DLOG(LogPostProcess, ELogLevel::Error,
                "ResolveShader failed: CreateDObject<DShader> returned nullptr (pass='{}', path='{}', expected valid DShader instance)",
                m_passName, fallbackPath.string());
            return nullptr;
        }
        m_fallbackShader->Initialize(fallbackPath, vsEntry, psEntry, vsProfile, psProfile);
    }

    if (!PostProcessShaderBlobValid(m_fallbackShader->GetVertexShaderBlob())
        || !PostProcessShaderBlobValid(m_fallbackShader->GetPixelShaderBlob()))
    {
        DLOG(LogPostProcess, ELogLevel::Error,
            "ResolveShader failed: compiled shader blobs missing or empty (pass='{}', path='{}', vsEntry='{}', psEntry='{}', expected non-empty VS and PS blobs)",
            m_passName, fallbackPath.string(), vsEntry, psEntry);
        return nullptr;
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
