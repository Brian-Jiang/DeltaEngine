#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include "Runtime/Core/DShader.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;

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

        // Root the fallback shader: it is referenced only by this raw pointer (not a
        // DPROPERTY), so without a root the GC would reclaim it as unreachable and leave
        // m_fallbackShader dangling.
        m_fallbackShaderRoot = m_fallbackShader->GetGCHandle();
        GetDObjectRegistry().AddRoot(m_fallbackShaderRoot);
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
    if (m_fallbackShaderRoot.IsSet())
    {
        DObjectRegistry& registry = GetDObjectRegistry();
        registry.RemoveRoot(m_fallbackShaderRoot);

        // Resolve through the registry so a shader already reclaimed by a final
        // shutdown sweep is not double-freed.
        if (DObject* shader = registry.Resolve(m_fallbackShaderRoot))
            GetReflectionRegistry().DestroyObject(shader);

        m_fallbackShaderRoot = {};
    }

    m_fallbackShader = nullptr;
}
