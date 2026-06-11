#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"

namespace DeltaEngine::Tests
{

DXRenderManager& GpuTestAssetFixture::GetRenderManager()
{
    auto manager = m_engine->GetRenderManager();
    EXPECT_NE(manager, nullptr);
    return *manager;
}

AssetId GpuTestAssetFixture::FindImportedAssetId(const std::string_view relativePath) const
{
    const std::filesystem::path path =
        std::filesystem::weakly_canonical(IOManager::GetEngineImportedAssetFullPath(relativePath));
    return m_assetDatabase->FindAssetIdByPath(path);
}

void GpuTestAssetFixture::SubmitAndFlush()
{
    const uint64_t fenceValue = GetDevice()->GetFrameFenceValue();
    GetDirectQueue().WaitForFenceValue(fenceValue);
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

PA_DScene* GpuTestAssetFixture::LoadImportedScene(const std::string_view relativePath)
{
    const AssetId sceneId = FindImportedAssetId(relativePath);
    if (sceneId.IsNull())
    {
        ADD_FAILURE() << "Imported scene not found: " << relativePath;
        return nullptr;
    }

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(sceneId);
    if (!sceneAsset)
    {
        ADD_FAILURE() << "Failed to load imported scene asset: " << relativePath;
        return nullptr;
    }

    m_engine->LoadScene(sceneAsset->GetAssetId());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
    return sceneAsset;
}

void GpuTestAssetFixture::RenderSceneFrames(const uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        RenderSceneFrame();
        AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
    }
}

std::shared_ptr<DirectX12Texture> GpuTestAssetFixture::GetFinalColorTexture() const
{
    const DXRenderManager* renderManager = m_engine ? m_engine->GetRenderManager().get() : nullptr;
    if (!renderManager)
        return nullptr;

    if (std::shared_ptr<DirectX12Texture> postProcessTexture = renderManager->GetFinalPostProcessTexture())
        return postProcessTexture;

    const std::shared_ptr<RenderTarget> renderTarget = GetRenderTarget();
    if (!renderTarget)
        return nullptr;

    return renderTarget->GetTexture(AttachmentPoint::Color0);
}

void GpuTestAssetFixture::ReinitializeRenderPipeline()
{
    m_engine->Cleanup();
    m_engine->CreateWorld();

    auto renderManager = CreateRenderManager();
    m_engine->Initialize(renderManager);
    m_engine->GetRenderManager()->InitWorldRenderers(*m_engine->GetWorld());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

void GpuTestAssetFixture::RenderSceneFrame()
{
    DXRenderManager& renderManager = GetRenderManager();
    renderManager.PrepareFrame();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());

    renderManager.RenderScene([this](const std::shared_ptr<DXGraphicsContext>& context)
    {
        m_engine->RecordSceneDraws(context);
    });
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());

    renderManager.RenderFrame();
    SubmitAndFlush();
}

void GpuTestAssetFixture::SetUp()
{
    const std::filesystem::path importedRoot = IOManager::GetEngineImportedAssetsFolder();
    if (!std::filesystem::is_directory(importedRoot))
        GTEST_SKIP() << "Engine imported assets folder unavailable: " << importedRoot.string();

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    m_locatorScope = std::make_unique<ScopedAssetDatabaseLocatorRegistration>(m_assetDatabase.get());
    m_assetDatabase->ScanAssetsFolder(importedRoot);

    m_engine = std::make_unique<EngineMain>();
    m_engine->CreateWorld();

    auto renderManager = CreateRenderManager();
    m_engine->Initialize(renderManager);
    m_engine->GetRenderManager()->InitWorldRenderers(*m_engine->GetWorld());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

void GpuTestAssetFixture::TearDown()
{
    if (m_engine)
        m_engine->Cleanup();

    m_locatorScope.reset();
    m_assetDatabase.reset();
    m_engine.reset();

    GpuGraphicsFixture::TearDown();
}

} // namespace DeltaEngine::Tests
