#pragma once

#include "Editor/EditorCore.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/EngineMain.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace DeltaEngine::Tests
{
inline DProperty* FindPropertyOnObject(DObject* object, const std::string& propertyName)
{
    if (!object || !object->GetClass())
        return nullptr;
    return object->GetClass()->FindPropertyByName(propertyName);
}

class EditorCoreFixture : public ::testing::Test
{
protected:
    std::filesystem::path m_tempDir;
    std::filesystem::path m_editorStateDir;
    std::filesystem::path m_defaultScenePath;
    std::unique_ptr<EngineMain> m_engine;
    std::unique_ptr<EditorCore> m_core;

    void SetUp() override
    {
        static std::atomic<uint64_t> s_counter{ 0 };
        const uint64_t n = s_counter.fetch_add(1, std::memory_order_relaxed);
        m_tempDir = std::filesystem::temp_directory_path() / ("DeltaEditorTest_" + std::to_string(n));
        std::filesystem::create_directories(m_tempDir);
        m_defaultScenePath = std::filesystem::weakly_canonical(m_tempDir / "DefaultScene.dasset.json");

        m_editorStateDir = IOManager::GetIntermediateFolder() / "EditorTests" / ("run_" + std::to_string(n));
        std::filesystem::create_directories(m_editorStateDir);
        IOManager::SetEditorStateFolderOverride(m_editorStateDir);

        m_engine = std::make_unique<EngineMain>();
        m_engine->CreateWorld();
        m_engine->Initialize(nullptr);

        m_core = std::make_unique<EditorCore>();
        m_core->Initialize(*m_engine, true, m_tempDir);
    }

    void TearDown() override
    {
        if (m_core)
            m_core->Shutdown();
        m_core.reset();
        m_engine.reset();
        IOManager::ClearEditorStateFolderOverride();
        std::error_code ec;
        std::filesystem::remove_all(m_editorStateDir, ec);
        std::filesystem::remove_all(m_tempDir, ec);
    }

    AssetId GetActiveSceneAssetId() const
    {
        DPrimaryAsset* pa = m_core->GetActiveSceneAsset();
        return pa ? pa->GetAssetId() : AssetId::Null();
    }
};

}
