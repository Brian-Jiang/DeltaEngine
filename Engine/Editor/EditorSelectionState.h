#pragma once

#include "EditorIncludes.h"
#include "Runtime/Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

class DComponent;
class DObject;
class EditorCore;
class GameObject;

class EditorSelectionState
{
public:
    EditorSelectionState() = default;

    DELTAEDITOR_API void SetSelection(const AssetId& assetId, const ObjectId& objectId);
    DELTAEDITOR_API void SelectAsset(const AssetId& assetId);
    DELTAEDITOR_API void ClearSelection();

    DELTAEDITOR_API bool HasSelection() const;
    DELTAEDITOR_API bool HasAssetSelection() const;

    DELTAEDITOR_API const AssetId& GetSelectedAssetId() const { return m_selectedAssetId; }
    DELTAEDITOR_API const ObjectId& GetSelectedObjectId() const { return m_selectedObjectId; }

    DELTAEDITOR_API DObject* ResolveSelection(EditorCore& core);
    DELTAEDITOR_API GameObject* GetSelectedGameObject(EditorCore& core);
    DELTAEDITOR_API DComponent* GetSelectedComponent(EditorCore& core);
    DELTAEDITOR_API GameObject* GetContextGameObject(EditorCore& core);

    DELTAEDITOR_API void NotifyObjectDestroyed(const ObjectId& objectId);

private:
    AssetId m_selectedAssetId = AssetId::Null();
    ObjectId m_selectedObjectId = ObjectId::Null();

    DObject* m_cachedObject = nullptr;
};

DELTA_ENGINE_NS_END
