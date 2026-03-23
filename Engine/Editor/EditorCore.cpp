#include "EditorCore.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/EngineMain.h"
#include "Runtime/IO/IOManager.h"

#include <cstdio>

using namespace DeltaEngine;

EditorCore* DeltaEngine::g_editorCore = nullptr;

EditorCore::EditorCore()
{
    g_editorCore = this;
}

EditorCore::~EditorCore()
{
    Shutdown();
    g_editorCore = nullptr;
}

void EditorCore::Initialize(EngineMain& engine)
{
    m_engine = &engine;

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    AssetDatabaseLocator::Register(m_assetDatabase.get());
    m_assetDatabase->ScanAssetsFolder(IOManager::GetEngineImportedAssetsFolder());

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(
        m_assetDatabase->FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath("DefaultScene")));
    if (sceneAsset)
        m_engine->LoadScene(sceneAsset->GetAssetId());
    else
        std::printf("Failed to load DefaultScene.\n");

    // CreateAssets();
    // m_engine->CreateGameObjects();
    // m_assetDatabase->SaveDirtyAssets();

    m_selectionState = std::make_unique<EditorSelectionState>();
}

void EditorCore::Shutdown()
{
    m_selectionState.reset();
    AssetDatabaseLocator::Unregister();
    m_assetDatabase.reset();
    m_engine = nullptr;
}

DWorld* EditorCore::GetWorld()
{
    return m_engine ? m_engine->GetWorld() : nullptr;
}

void EditorCore::LoadScene(const std::filesystem::path& scenePath)
{
    if (!m_assetDatabase || !m_engine)
        return;

    AssetId id = m_assetDatabase->FindAssetIdByPath(scenePath);
    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(id);
    if (sceneAsset)
        m_engine->LoadScene(sceneAsset->GetAssetId());
}

// void EditorMain::CreateAssets()
//{
//     // Default scene
//     const std::filesystem::path scenePath = IOManager::GetEngineImportedAssetFullPath("DefaultScene");
//
//     PA_DScene* sceneAsset = PA_DScene::Create("DefaultScene");
//     m_assetDatabase->CreateAsset(scenePath, sceneAsset);
//     AssetId sceneId = sceneAsset->GetAssetId();
//
//     // Default shader
//     DShader* shader = CreateDObject<DShader>();
//     {
//         shader->Initialize(
//             L"Shaders.hlsl",
//             L"VSMain", L"PSMain",
//             L"vs_6_0", L"ps_6_0");
//
//         shader->SetInputLayout({
//             { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//         });
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("DefaultShader"),
//             PA_Shader::Create(shader));
//     }
//
//     // Star mesh and material
//     {
//         DMesh* mesh = CreateDObject<DMesh>();
//         mesh->Initialize(std::wstring(L"Star.obj"));
//         std::vector<DTexture*> textures = mesh->GetTextures();
//         std::vector<DMaterial*> materials;
//         for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//             DMaterial* material = CreateDObject<DMaterial>();
//             material->Initialize(shader);
//             if (i < textures.size()) {
//                 m_assetDatabase->CreateAsset(
//                     IOManager::GetEngineImportedAssetFullPath("StarTexture_" + std::to_string(i)),
//                     PA_Texture::Create(textures[i]));
//                 material->AddTexture(textures[i]);
//             }
//             materials.push_back(material);
//
//             m_assetDatabase->CreateAsset(
//                 IOManager::GetEngineImportedAssetFullPath("StarMaterial_" + std::to_string(i)),
//                 PA_Material::Create(material));
//         }
//
//         mesh->SetMaterials(materials);
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("StarMesh"),
//             PA_StaticMesh::Create(mesh));
//     }
//
//     // Home mesh and material
//     {
//         DMesh* mesh = CreateDObject<DMesh>();
//         mesh->Initialize(std::wstring(L"home/source/home.fbx"));
//         std::vector<DTexture*> textures = mesh->GetTextures();
//         std::vector<DMaterial*> materials;
//         for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//             DMaterial* material = CreateDObject<DMaterial>();
//             material->Initialize(shader);
//             if (i < textures.size()) {
//                 m_assetDatabase->CreateAsset(
//                     IOManager::GetEngineImportedAssetFullPath("HomeTexture_" + std::to_string(i)),
//                     PA_Texture::Create(textures[i]));
//                 material->AddTexture(textures[i]);
//             }
//             materials.push_back(material);
//
//             m_assetDatabase->CreateAsset(
//                 IOManager::GetEngineImportedAssetFullPath("HomeMaterial_" + std::to_string(i)),
//                 PA_Material::Create(material));
//         }
//
//         mesh->SetMaterials(materials);
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("HomeMesh"),
//             PA_StaticMesh::Create(mesh));
//     }
// }
