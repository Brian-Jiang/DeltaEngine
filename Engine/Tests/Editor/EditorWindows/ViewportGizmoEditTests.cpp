// Engine/Tests/Editor/EditorWindows/ViewportGizmoEditTests.cpp
#include "Editor/EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#ifdef UUID
#undef UUID
#endif
#include <nlohmann/json.hpp>

#include <cmath>
#include <string>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;
using namespace DirectX::SimpleMath;
using DeltaEngine::UUID;

// Exercises the commit path used by the viewport ImGuizmo drag without an ImGui context:
// the drag itself only calls SetLocalPosition/Rotation/Scale, and the undo entry comes from
// CaptureGizmoTransformValue at edit-begin plus CommitGizmoTransformEdit at edit-end.
class ViewportGizmoEditTests : public EditorCoreFixture
{
protected:
    SceneComponent* CreateSceneComponent()
    {
        const std::string goId = ExecCreateGameObject();
        if (goId.empty())
            return nullptr;
        const std::string scId = ExecCreateComponent(goId, "SceneComponent");
        if (scId.empty())
            return nullptr;
        return m_core->ResolveObject<SceneComponent>(GetActiveSceneAssetId(), UUID::FromString(scId));
    }

    static bool Vec3Near(const Vector3& a, const Vector3& b, float eps = 1e-3f)
    {
        return std::fabs(a.x - b.x) <= eps && std::fabs(a.y - b.y) <= eps && std::fabs(a.z - b.z) <= eps;
    }
};

TEST_F(ViewportGizmoEditTests, PropertyName_MapsToolToDecomposedProperty)
{
    EXPECT_STREQ(GizmoTransformPropertyName(EEditorTransformTool::Move), "m_localPosition");
    EXPECT_STREQ(GizmoTransformPropertyName(EEditorTransformTool::Rotate), "m_localEulerAngles");
    EXPECT_STREQ(GizmoTransformPropertyName(EEditorTransformTool::Scale), "m_localScale");
    EXPECT_EQ(GizmoTransformPropertyName(EEditorTransformTool::Select), nullptr);
}

TEST_F(ViewportGizmoEditTests, TranslateDrag_ProducesUndoableEntry)
{
    SceneComponent* sc = CreateSceneComponent();
    ASSERT_NE(sc, nullptr);
    sc->SetLocalPosition(Vector3(1.0f, 2.0f, 3.0f));

    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();
    nlohmann::json before = CaptureGizmoTransformValue(sc, EEditorTransformTool::Move);

    sc->SetLocalPosition(Vector3(10.0f, -4.0f, 0.5f));
    ASSERT_TRUE(CommitGizmoTransformEdit(*m_core, sc, EEditorTransformTool::Move, before));

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore + 1);
    EXPECT_TRUE(Vec3Near(sc->GetLocalPosition(), Vector3(10.0f, -4.0f, 0.5f)));

    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_TRUE(Vec3Near(sc->GetLocalPosition(), Vector3(1.0f, 2.0f, 3.0f)));
}

TEST_F(ViewportGizmoEditTests, RotateDrag_CommitsEulerAndRebuildsQuaternionOnUndo)
{
    SceneComponent* sc = CreateSceneComponent();
    ASSERT_NE(sc, nullptr);
    sc->SetLocalRotation(Vector3(0.0f, 30.0f, 0.0f));

    const Quaternion rotationBefore = sc->GetLocalRotation();
    nlohmann::json before = CaptureGizmoTransformValue(sc, EEditorTransformTool::Rotate);

    // Mirrors the live drag, which feeds the decomposed quaternion back through SetLocalRotation.
    sc->SetLocalRotation(Quaternion::CreateFromYawPitchRoll(
        DirectX::XMConvertToRadians(75.0f), 0.0f, 0.0f));
    ASSERT_TRUE(CommitGizmoTransformEdit(*m_core, sc, EEditorTransformTool::Rotate, before));

    EXPECT_NEAR(sc->GetLocalRotationEulerAngles().y, 75.0f, 1e-2f);

    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_NEAR(sc->GetLocalRotationEulerAngles().y, 30.0f, 1e-2f);

    // Undoing the euler property must also restore the quaternion via PostEditChangeProperty.
    const Quaternion restored = sc->GetLocalRotation();
    EXPECT_NEAR(std::fabs(restored.x * rotationBefore.x + restored.y * rotationBefore.y +
                          restored.z * rotationBefore.z + restored.w * rotationBefore.w),
                1.0f, 1e-3f);
}

TEST_F(ViewportGizmoEditTests, ScaleDrag_ProducesUndoableEntry)
{
    SceneComponent* sc = CreateSceneComponent();
    ASSERT_NE(sc, nullptr);

    nlohmann::json before = CaptureGizmoTransformValue(sc, EEditorTransformTool::Scale);
    sc->SetLocalScale(Vector3(2.0f, 2.0f, 2.0f));
    ASSERT_TRUE(CommitGizmoTransformEdit(*m_core, sc, EEditorTransformTool::Scale, before));

    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_TRUE(Vec3Near(sc->GetLocalScale(), Vector3(1.0f, 1.0f, 1.0f)));
}

TEST_F(ViewportGizmoEditTests, DragWithoutChange_EmitsNoUndoEntry)
{
    SceneComponent* sc = CreateSceneComponent();
    ASSERT_NE(sc, nullptr);

    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();
    nlohmann::json before = CaptureGizmoTransformValue(sc, EEditorTransformTool::Move);

    EXPECT_FALSE(CommitGizmoTransformEdit(*m_core, sc, EEditorTransformTool::Move, before));
    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
}
