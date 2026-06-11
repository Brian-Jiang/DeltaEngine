#pragma once

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/IO/IOManager.h"
#include "Shared/GpuGraphicsFixture.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace DeltaEngine::Tests
{

class GpuTestAssetFixture : public GpuGraphicsFixture
{
protected:
    void SetUp() override;
    void TearDown() override;

    EngineMain& GetEngine() { return *m_engine; }
    DXRenderManager& GetRenderManager();
    EditorAssetDatabase& GetAssetDatabase() { return *m_assetDatabase; }

    AssetId FindImportedAssetId(std::string_view relativePath) const;
    PA_DScene* LoadImportedScene(std::string_view relativePath);
    void RenderSceneFrame();
    void RenderSceneFrames(uint32_t count);
    void SubmitAndFlush();
    void ReinitializeRenderPipeline();
    std::shared_ptr<DirectX12Texture> GetFinalColorTexture() const;

private:
    struct ScopedAssetDatabaseLocatorRegistration
    {
        explicit ScopedAssetDatabaseLocatorRegistration(IAssetDatabase* db) { AssetDatabaseLocator::Register(db); }
        ~ScopedAssetDatabaseLocatorRegistration() { AssetDatabaseLocator::Unregister(); }
    };

    std::unique_ptr<EditorAssetDatabase> m_assetDatabase;
    std::unique_ptr<ScopedAssetDatabaseLocatorRegistration> m_locatorScope;
    std::unique_ptr<EngineMain> m_engine;
};

} // namespace DeltaEngine::Tests
