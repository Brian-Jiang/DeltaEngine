#pragma once

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/IO/IOManager.h"
#include "Shared/GpuGraphicsFixture.h"

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
    void RenderSceneFrame();
    void SubmitAndFlush();

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
