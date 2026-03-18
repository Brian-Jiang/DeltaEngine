#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <string>

DELTA_ENGINE_NS_BEGIN

class DScene;
class DPrimaryAsset;

/// Utility helpers for creating and inspecting a DPrimaryAsset that contains
/// a DScene as its root object.
///
/// PA_DScene is not a reflected type — it is a thin factory/accessor layer.
/// The underlying storage is always a plain DPrimaryAsset.
class DELTAENGINE_API PA_DScene
{
public:
    PA_DScene() = delete;

    /// Creates a new DPrimaryAsset that owns a single DScene object.
    /// The caller is responsible for registering the returned asset with
    /// the asset database (via EditorAssetDatabase::CreateAsset or similar).
    static DPrimaryAsset* Create(const std::string& sceneName);

    /// Returns the DScene object stored inside the given primary asset,
    /// or nullptr if the asset does not contain a DScene.
    static DScene* GetScene(DPrimaryAsset* asset);
};

DELTA_ENGINE_NS_END
